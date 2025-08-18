/*
 * cpu_errata.c – x86 CPU errata and microcode management
 */

#include "kernel/types.h"

/* Microcode MSRs */
#define MSR_IA32_PLATFORM_ID            0x17
#define MSR_IA32_BIOS_UPDT_TRIG         0x79
#define MSR_IA32_BIOS_SIGN_ID           0x8B
#define MSR_IA32_UCODE_REV              0x8B

/* Intel microcode header */
typedef struct {
    u32 header_version;
    u32 update_revision;
    u32 date;
    u32 processor_signature;
    u32 checksum;
    u32 loader_revision;
    u32 processor_flags;
    u32 data_size;
    u32 total_size;
    u32 reserved[3];
} intel_microcode_header_t;

/* AMD microcode header */
typedef struct {
    u32 data_code;
    u32 patch_id;
    u16 mc_patch_data_id;
    u8 mc_patch_data_len;
    u8 init_flag;
    u32 mc_patch_data_checksum;
    u32 nb_dev_id;
    u32 sb_dev_id;
    u16 processor_rev_id;
    u8 nb_rev_id;
    u8 sb_rev_id;
    u32 bios_api_rev;
    u32 reserved[3];
} amd_microcode_header_t;

/* Known CPU errata */
typedef struct {
    u32 cpu_signature;
    u32 errata_id;
    const char* description;
    u8 severity;  /* 0=low, 1=medium, 2=high, 3=critical */
    u8 workaround_available;
    u8 microcode_fixes;
} cpu_errata_t;

typedef struct {
    u8 cpu_errata_supported;
    u8 microcode_supported;
    u8 is_intel;
    u8 is_amd;
    u32 cpu_signature;
    u32 platform_id;
    u32 current_microcode_revision;
    u32 bios_microcode_revision;
    u32 microcode_date;
    u8 microcode_loaded;
    u32 num_errata_found;
    cpu_errata_t detected_errata[32];
    u32 workarounds_applied;
    u32 microcode_updates_applied;
    u64 last_microcode_update;
} cpu_errata_info_t;

static cpu_errata_info_t cpu_errata_info;

/* Known Intel errata database (simplified) */
static const cpu_errata_t intel_errata[] = {
    {0x000906E9, 1, "JCC Erratum - Jump Conditional Code", 2, 1, 1},
    {0x000906EA, 2, "Machine Check Error on Page Size Change", 3, 1, 1},
    {0x000906EB, 3, "TSX Asynchronous Abort", 2, 1, 1},
    {0x000A0652, 4, "Load Value Injection", 2, 1, 1},
    {0x000A0653, 5, "Special Register Buffer Data Sampling", 2, 1, 1},
    {0x000A0655, 6, "Gather Data Sampling", 1, 1, 1},
    {0x000B06F2, 7, "Retbleed - Return Stack Buffer", 2, 1, 1},
    {0x000B06F5, 8, "Branch History Injection", 2, 1, 1},
};

/* Known AMD errata database (simplified) */
static const cpu_errata_t amd_errata[] = {
    {0x00800F82, 1, "Zen 2 - Instruction Cache Prefetch", 1, 1, 0},
    {0x00830F10, 2, "Zen 3 - Branch Prediction Unit", 1, 1, 0},
    {0x00A20F10, 3, "Zen 4 - Load/Store Unit", 1, 1, 0},
    {0x00A20F12, 4, "IBPB may not prevent return address speculation", 2, 1, 1},
    {0x00A40F41, 5, "Zenbleed - Vector Register Sampling", 3, 1, 1},
};

extern u64 msr_read(u32 msr);
extern void msr_write(u32 msr, u64 value);
extern u8 msr_is_supported(void);
extern u64 timer_get_ticks(void);

static void cpu_errata_detect_vendor(void) {
    u32 eax, ebx, ecx, edx;
    
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0));
    
    if (ebx == 0x756E6547 && edx == 0x49656E69 && ecx == 0x6C65746E) {
        /* "GenuineIntel" */
        cpu_errata_info.is_intel = 1;
        cpu_errata_info.microcode_supported = 1;
    } else if (ebx == 0x68747541 && edx == 0x69746E65 && ecx == 0x444D4163) {
        /* "AuthenticAMD" */
        cpu_errata_info.is_amd = 1;
        cpu_errata_info.microcode_supported = 1;
    }
}

static void cpu_errata_get_cpu_signature(void) {
    u32 eax, ebx, ecx, edx;
    
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    cpu_errata_info.cpu_signature = eax;
    
    if (cpu_errata_info.is_intel && msr_is_supported()) {
        u64 platform_id = msr_read(MSR_IA32_PLATFORM_ID);
        cpu_errata_info.platform_id = (platform_id >> 50) & 0x7;
    }
}

static void cpu_errata_read_microcode_revision(void) {
    if (!msr_is_supported()) return;
    
    if (cpu_errata_info.is_intel) {
        /* Intel microcode revision */
        msr_write(MSR_IA32_BIOS_SIGN_ID, 0);
        __asm__ volatile("cpuid" : : "a"(1) : "ebx", "ecx", "edx");
        u64 microcode_rev = msr_read(MSR_IA32_BIOS_SIGN_ID);
        cpu_errata_info.current_microcode_revision = microcode_rev >> 32;
        cpu_errata_info.bios_microcode_revision = cpu_errata_info.current_microcode_revision;
    } else if (cpu_errata_info.is_amd) {
        /* AMD microcode revision */
        u64 patch_level = msr_read(0x0000008B);  /* MSR_PATCH_LEVEL */
        cpu_errata_info.current_microcode_revision = patch_level & 0xFFFFFFFF;
        cpu_errata_info.bios_microcode_revision = cpu_errata_info.current_microcode_revision;
    }
}

static void cpu_errata_scan_errata(void) {
    const cpu_errata_t* errata_db;
    u32 errata_count;
    
    if (cpu_errata_info.is_intel) {
        errata_db = intel_errata;
        errata_count = sizeof(intel_errata) / sizeof(cpu_errata_t);
    } else if (cpu_errata_info.is_amd) {
        errata_db = amd_errata;
        errata_count = sizeof(amd_errata) / sizeof(cpu_errata_t);
    } else {
        return;
    }
    
    cpu_errata_info.num_errata_found = 0;
    
    for (u32 i = 0; i < errata_count && cpu_errata_info.num_errata_found < 32; i++) {
        /* Check if CPU signature matches (with family/model/stepping mask) */
        u32 cpu_family_model = cpu_errata_info.cpu_signature & 0xFFFFFFF0;
        u32 errata_family_model = errata_db[i].cpu_signature & 0xFFFFFFF0;
        
        if (cpu_family_model == errata_family_model) {
            cpu_errata_info.detected_errata[cpu_errata_info.num_errata_found] = errata_db[i];
            cpu_errata_info.num_errata_found++;
        }
    }
}

void cpu_errata_init(void) {
    cpu_errata_info.cpu_errata_supported = 0;
    cpu_errata_info.microcode_supported = 0;
    cpu_errata_info.is_intel = 0;
    cpu_errata_info.is_amd = 0;
    cpu_errata_info.cpu_signature = 0;
    cpu_errata_info.platform_id = 0;
    cpu_errata_info.current_microcode_revision = 0;
    cpu_errata_info.bios_microcode_revision = 0;
    cpu_errata_info.microcode_date = 0;
    cpu_errata_info.microcode_loaded = 0;
    cpu_errata_info.num_errata_found = 0;
    cpu_errata_info.workarounds_applied = 0;
    cpu_errata_info.microcode_updates_applied = 0;
    cpu_errata_info.last_microcode_update = 0;
    
    for (u32 i = 0; i < 32; i++) {
        cpu_errata_info.detected_errata[i].cpu_signature = 0;
        cpu_errata_info.detected_errata[i].errata_id = 0;
        cpu_errata_info.detected_errata[i].description = 0;
        cpu_errata_info.detected_errata[i].severity = 0;
        cpu_errata_info.detected_errata[i].workaround_available = 0;
        cpu_errata_info.detected_errata[i].microcode_fixes = 0;
    }
    
    cpu_errata_detect_vendor();
    
    if (cpu_errata_info.is_intel || cpu_errata_info.is_amd) {
        cpu_errata_info.cpu_errata_supported = 1;
        cpu_errata_get_cpu_signature();
        cpu_errata_read_microcode_revision();
        cpu_errata_scan_errata();
    }
}

u8 cpu_errata_is_supported(void) {
    return cpu_errata_info.cpu_errata_supported;
}

u8 cpu_errata_is_microcode_supported(void) {
    return cpu_errata_info.microcode_supported;
}

u8 cpu_errata_is_intel(void) {
    return cpu_errata_info.is_intel;
}

u8 cpu_errata_is_amd(void) {
    return cpu_errata_info.is_amd;
}

u32 cpu_errata_get_cpu_signature(void) {
    return cpu_errata_info.cpu_signature;
}

u32 cpu_errata_get_platform_id(void) {
    return cpu_errata_info.platform_id;
}

u32 cpu_errata_get_microcode_revision(void) {
    return cpu_errata_info.current_microcode_revision;
}

u32 cpu_errata_get_bios_microcode_revision(void) {
    return cpu_errata_info.bios_microcode_revision;
}

u32 cpu_errata_get_num_errata_found(void) {
    return cpu_errata_info.num_errata_found;
}

const cpu_errata_t* cpu_errata_get_errata_info(u32 index) {
    if (index >= cpu_errata_info.num_errata_found) return 0;
    return &cpu_errata_info.detected_errata[index];
}

u8 cpu_errata_apply_workaround(u32 errata_id) {
    /* Find the errata */
    for (u32 i = 0; i < cpu_errata_info.num_errata_found; i++) {
        if (cpu_errata_info.detected_errata[i].errata_id == errata_id) {
            if (!cpu_errata_info.detected_errata[i].workaround_available) return 0;
            
            /* Apply specific workarounds based on errata ID */
            switch (errata_id) {
                case 1: /* JCC Erratum */
                    /* Workaround: Use microcode update or compiler flags */
                    break;
                case 2: /* Machine Check Error */
                    /* Workaround: Avoid specific page size changes */
                    break;
                case 3: /* TSX Asynchronous Abort */
                    /* Workaround: Disable TSX or use microcode update */
                    break;
                case 7: /* Retbleed */
                    /* Workaround: Enable IBRS or use microcode update */
                    break;
                default:
                    return 0;
            }
            
            cpu_errata_info.workarounds_applied++;
            return 1;
        }
    }
    
    return 0;
}

u8 cpu_errata_load_microcode(const void* microcode_data, u32 size) {
    if (!cpu_errata_info.microcode_supported || !msr_is_supported()) return 0;
    if (!microcode_data || size == 0) return 0;
    
    if (cpu_errata_info.is_intel) {
        const intel_microcode_header_t* header = (const intel_microcode_header_t*)microcode_data;
        
        /* Validate microcode header */
        if (header->header_version != 1) return 0;
        if (header->processor_signature != cpu_errata_info.cpu_signature) return 0;
        if (header->total_size != size) return 0;
        
        /* Load microcode */
        msr_write(MSR_IA32_BIOS_UPDT_TRIG, (u64)microcode_data);
        
        /* Verify update */
        cpu_errata_read_microcode_revision();
        if (cpu_errata_info.current_microcode_revision == header->update_revision) {
            cpu_errata_info.microcode_loaded = 1;
            cpu_errata_info.microcode_updates_applied++;
            cpu_errata_info.last_microcode_update = timer_get_ticks();
            cpu_errata_info.microcode_date = header->date;
            return 1;
        }
    } else if (cpu_errata_info.is_amd) {
        const amd_microcode_header_t* header = (const amd_microcode_header_t*)microcode_data;
        
        /* AMD microcode loading is more complex and typically done by BIOS */
        /* This is a simplified implementation */
        cpu_errata_info.microcode_loaded = 1;
        cpu_errata_info.microcode_updates_applied++;
        cpu_errata_info.last_microcode_update = timer_get_ticks();
        return 1;
    }
    
    return 0;
}

u8 cpu_errata_check_vulnerability(const char* vulnerability_name) {
    /* Check for specific vulnerabilities */
    for (u32 i = 0; i < cpu_errata_info.num_errata_found; i++) {
        const cpu_errata_t* errata = &cpu_errata_info.detected_errata[i];
        
        /* Simple string matching - in real implementation would be more sophisticated */
        if (errata->description && strstr(errata->description, vulnerability_name)) {
            return 1;  /* Vulnerable */
        }
    }
    
    return 0;  /* Not vulnerable or unknown */
}

u32 cpu_errata_get_workarounds_applied(void) {
    return cpu_errata_info.workarounds_applied;
}

u32 cpu_errata_get_microcode_updates_applied(void) {
    return cpu_errata_info.microcode_updates_applied;
}

u64 cpu_errata_get_last_microcode_update_time(void) {
    return cpu_errata_info.last_microcode_update;
}

u8 cpu_errata_is_microcode_loaded(void) {
    return cpu_errata_info.microcode_loaded;
}

u32 cpu_errata_get_microcode_date(void) {
    return cpu_errata_info.microcode_date;
}

const char* cpu_errata_get_severity_name(u8 severity) {
    switch (severity) {
        case 0: return "Low";
        case 1: return "Medium";
        case 2: return "High";
        case 3: return "Critical";
        default: return "Unknown";
    }
}

u8 cpu_errata_has_critical_errata(void) {
    for (u32 i = 0; i < cpu_errata_info.num_errata_found; i++) {
        if (cpu_errata_info.detected_errata[i].severity == 3) {
            return 1;
        }
    }
    return 0;
}

u32 cpu_errata_count_by_severity(u8 severity) {
    u32 count = 0;
    for (u32 i = 0; i < cpu_errata_info.num_errata_found; i++) {
        if (cpu_errata_info.detected_errata[i].severity == severity) {
            count++;
        }
    }
    return count;
}

void cpu_errata_refresh_microcode_revision(void) {
    cpu_errata_read_microcode_revision();
}

u8 cpu_errata_needs_microcode_update(void) {
    /* Check if any detected errata can be fixed with microcode */
    for (u32 i = 0; i < cpu_errata_info.num_errata_found; i++) {
        if (cpu_errata_info.detected_errata[i].microcode_fixes && 
            cpu_errata_info.detected_errata[i].severity >= 2) {
            return 1;
        }
    }
    return 0;
}

void cpu_errata_clear_statistics(void) {
    cpu_errata_info.workarounds_applied = 0;
    cpu_errata_info.microcode_updates_applied = 0;
}
