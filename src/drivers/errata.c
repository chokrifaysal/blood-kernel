/*
 * errata.c – x86 CPU errata detection and workarounds
 */

#include "kernel/types.h"

/* Known CPU families and models */
#define INTEL_FAMILY_6          0x6
#define AMD_FAMILY_15           0x15
#define AMD_FAMILY_16           0x16
#define AMD_FAMILY_17           0x17
#define AMD_FAMILY_19           0x19

/* Intel model numbers */
#define INTEL_MODEL_NEHALEM     0x1A
#define INTEL_MODEL_WESTMERE    0x25
#define INTEL_MODEL_SANDYBRIDGE 0x2A
#define INTEL_MODEL_IVYBRIDGE   0x3A
#define INTEL_MODEL_HASWELL     0x3C
#define INTEL_MODEL_BROADWELL   0x3D
#define INTEL_MODEL_SKYLAKE     0x4E
#define INTEL_MODEL_KABYLAKE    0x8E
#define INTEL_MODEL_COFFEELAKE  0x9E
#define INTEL_MODEL_ICELAKE     0x7E

/* Errata types */
#define ERRATA_TYPE_PERFORMANCE 0x01
#define ERRATA_TYPE_SECURITY    0x02
#define ERRATA_TYPE_FUNCTIONAL  0x04
#define ERRATA_TYPE_CRITICAL    0x08

typedef struct {
    u32 cpu_signature;
    u16 errata_id;
    u8 errata_type;
    u8 severity;
    const char* description;
    void (*workaround)(void);
} errata_entry_t;

typedef struct {
    u32 cpu_signature;
    u32 family;
    u32 model;
    u32 stepping;
    u8 vendor_intel;
    u8 vendor_amd;
    u32 active_errata_count;
    u16 active_errata[64];
} errata_info_t;

static errata_info_t errata_info;

extern u64 msr_read(u32 msr);
extern void msr_write(u32 msr, u64 value);
extern u8 msr_is_supported(void);

/* Workaround functions */
static void errata_intel_spectre_v1_workaround(void) {
    /* Spectre variant 1 mitigation */
    __asm__ volatile("lfence" : : : "memory");
}

static void errata_intel_spectre_v2_workaround(void) {
    /* Spectre variant 2 mitigation - IBRS */
    if (msr_is_supported()) {
        u64 spec_ctrl = msr_read(0x48);
        spec_ctrl |= 0x01; /* IBRS */
        msr_write(0x48, spec_ctrl);
    }
}

static void errata_intel_meltdown_workaround(void) {
    /* Meltdown mitigation - KPTI would be implemented here */
    /* This is a placeholder for page table isolation */
}

static void errata_intel_l1tf_workaround(void) {
    /* L1 Terminal Fault mitigation */
    if (msr_is_supported()) {
        u64 flush_cmd = msr_read(0x10B);
        flush_cmd |= 0x01; /* L1D_FLUSH */
        msr_write(0x10B, flush_cmd);
    }
}

static void errata_intel_mds_workaround(void) {
    /* Microarchitectural Data Sampling mitigation */
    __asm__ volatile("verw %%ax" : : "a"(0) : "memory");
}

static void errata_amd_branch_predictor_workaround(void) {
    /* AMD branch predictor errata workaround */
    if (msr_is_supported()) {
        u64 de_cfg = msr_read(0xC0011029);
        de_cfg |= (1ULL << 9); /* Disable branch predictor */
        msr_write(0xC0011029, de_cfg);
    }
}

static void errata_intel_tsx_workaround(void) {
    /* Intel TSX errata workaround */
    if (msr_is_supported()) {
        u64 tsx_ctrl = msr_read(0x122);
        tsx_ctrl |= 0x02; /* TSX_CTRL_RTM_DISABLE */
        msr_write(0x122, tsx_ctrl);
    }
}

static void errata_intel_srbds_workaround(void) {
    /* Special Register Buffer Data Sampling workaround */
    if (msr_is_supported()) {
        u64 mcuopt_ctrl = msr_read(0x123);
        mcuopt_ctrl |= 0x01; /* RNGDS_MITG_DIS */
        msr_write(0x123, mcuopt_ctrl);
    }
}

/* Errata database */
static const errata_entry_t errata_database[] = {
    /* Intel Spectre/Meltdown */
    {0x000306A0, 1001, ERRATA_TYPE_SECURITY, 9, "Spectre Variant 1", errata_intel_spectre_v1_workaround},
    {0x000306A0, 1002, ERRATA_TYPE_SECURITY, 9, "Spectre Variant 2", errata_intel_spectre_v2_workaround},
    {0x000306A0, 1003, ERRATA_TYPE_SECURITY, 10, "Meltdown", errata_intel_meltdown_workaround},
    
    /* Intel L1TF */
    {0x000306F0, 1004, ERRATA_TYPE_SECURITY, 8, "L1 Terminal Fault", errata_intel_l1tf_workaround},
    {0x000406E0, 1004, ERRATA_TYPE_SECURITY, 8, "L1 Terminal Fault", errata_intel_l1tf_workaround},
    
    /* Intel MDS */
    {0x000306F0, 1005, ERRATA_TYPE_SECURITY, 7, "Microarchitectural Data Sampling", errata_intel_mds_workaround},
    {0x000406E0, 1005, ERRATA_TYPE_SECURITY, 7, "Microarchitectural Data Sampling", errata_intel_mds_workaround},
    {0x000506E0, 1005, ERRATA_TYPE_SECURITY, 7, "Microarchitectural Data Sampling", errata_intel_mds_workaround},
    
    /* Intel TSX */
    {0x000306F0, 1006, ERRATA_TYPE_FUNCTIONAL, 6, "TSX Errata", errata_intel_tsx_workaround},
    {0x000406E0, 1006, ERRATA_TYPE_FUNCTIONAL, 6, "TSX Errata", errata_intel_tsx_workaround},
    
    /* Intel SRBDS */
    {0x000506E0, 1007, ERRATA_TYPE_SECURITY, 5, "Special Register Buffer Data Sampling", errata_intel_srbds_workaround},
    {0x000806E0, 1007, ERRATA_TYPE_SECURITY, 5, "Special Register Buffer Data Sampling", errata_intel_srbds_workaround},
    
    /* AMD Branch Predictor */
    {0x00630F00, 2001, ERRATA_TYPE_PERFORMANCE, 4, "Branch Predictor Errata", errata_amd_branch_predictor_workaround},
    {0x00730F00, 2001, ERRATA_TYPE_PERFORMANCE, 4, "Branch Predictor Errata", errata_amd_branch_predictor_workaround},
};

static void errata_detect_cpu_info(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Get CPU signature */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    errata_info.cpu_signature = eax;
    
    /* Extract family, model, stepping */
    errata_info.stepping = eax & 0xF;
    errata_info.model = (eax >> 4) & 0xF;
    errata_info.family = (eax >> 8) & 0xF;
    
    /* Handle extended family/model */
    if (errata_info.family == 0xF) {
        errata_info.family += (eax >> 20) & 0xFF;
    }
    if (errata_info.family == 0x6 || errata_info.family == 0xF) {
        errata_info.model += ((eax >> 16) & 0xF) << 4;
    }
    
    /* Detect vendor */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0));
    
    if (ebx == 0x756E6547 && edx == 0x49656E69 && ecx == 0x6C65746E) {
        errata_info.vendor_intel = 1;
        errata_info.vendor_amd = 0;
    } else if (ebx == 0x68747541 && edx == 0x69746E65 && ecx == 0x444D4163) {
        errata_info.vendor_intel = 0;
        errata_info.vendor_amd = 1;
    } else {
        errata_info.vendor_intel = 0;
        errata_info.vendor_amd = 0;
    }
}

static void errata_scan_and_apply(void) {
    errata_info.active_errata_count = 0;
    
    u32 database_size = sizeof(errata_database) / sizeof(errata_entry_t);
    
    for (u32 i = 0; i < database_size; i++) {
        const errata_entry_t* entry = &errata_database[i];
        
        /* Check if errata applies to this CPU */
        u32 cpu_mask = entry->cpu_signature & 0xFFFFF0F0; /* Mask stepping */
        u32 our_mask = errata_info.cpu_signature & 0xFFFFF0F0;
        
        if (cpu_mask == our_mask) {
            /* Apply workaround */
            if (entry->workaround) {
                entry->workaround();
            }
            
            /* Record active errata */
            if (errata_info.active_errata_count < 64) {
                errata_info.active_errata[errata_info.active_errata_count] = entry->errata_id;
                errata_info.active_errata_count++;
            }
        }
    }
}

void errata_init(void) {
    errata_info.cpu_signature = 0;
    errata_info.family = 0;
    errata_info.model = 0;
    errata_info.stepping = 0;
    errata_info.vendor_intel = 0;
    errata_info.vendor_amd = 0;
    errata_info.active_errata_count = 0;
    
    errata_detect_cpu_info();
    errata_scan_and_apply();
}

u32 errata_get_cpu_signature(void) {
    return errata_info.cpu_signature;
}

u32 errata_get_family(void) {
    return errata_info.family;
}

u32 errata_get_model(void) {
    return errata_info.model;
}

u32 errata_get_stepping(void) {
    return errata_info.stepping;
}

u8 errata_is_intel(void) {
    return errata_info.vendor_intel;
}

u8 errata_is_amd(void) {
    return errata_info.vendor_amd;
}

u32 errata_get_active_count(void) {
    return errata_info.active_errata_count;
}

u16 errata_get_active_id(u32 index) {
    if (index >= errata_info.active_errata_count) return 0;
    return errata_info.active_errata[index];
}

const char* errata_get_description(u16 errata_id) {
    u32 database_size = sizeof(errata_database) / sizeof(errata_entry_t);
    
    for (u32 i = 0; i < database_size; i++) {
        if (errata_database[i].errata_id == errata_id) {
            return errata_database[i].description;
        }
    }
    
    return "Unknown errata";
}

u8 errata_get_type(u16 errata_id) {
    u32 database_size = sizeof(errata_database) / sizeof(errata_entry_t);
    
    for (u32 i = 0; i < database_size; i++) {
        if (errata_database[i].errata_id == errata_id) {
            return errata_database[i].errata_type;
        }
    }
    
    return 0;
}

u8 errata_get_severity(u16 errata_id) {
    u32 database_size = sizeof(errata_database) / sizeof(errata_entry_t);
    
    for (u32 i = 0; i < database_size; i++) {
        if (errata_database[i].errata_id == errata_id) {
            return errata_database[i].severity;
        }
    }
    
    return 0;
}

u8 errata_is_security_related(u16 errata_id) {
    return (errata_get_type(errata_id) & ERRATA_TYPE_SECURITY) != 0;
}

u8 errata_is_performance_related(u16 errata_id) {
    return (errata_get_type(errata_id) & ERRATA_TYPE_PERFORMANCE) != 0;
}

u8 errata_is_functional_related(u16 errata_id) {
    return (errata_get_type(errata_id) & ERRATA_TYPE_FUNCTIONAL) != 0;
}

u8 errata_is_critical(u16 errata_id) {
    return (errata_get_type(errata_id) & ERRATA_TYPE_CRITICAL) != 0;
}

void errata_apply_workaround(u16 errata_id) {
    u32 database_size = sizeof(errata_database) / sizeof(errata_entry_t);
    
    for (u32 i = 0; i < database_size; i++) {
        if (errata_database[i].errata_id == errata_id) {
            if (errata_database[i].workaround) {
                errata_database[i].workaround();
            }
            return;
        }
    }
}

u8 errata_check_vulnerability(const char* vuln_name) {
    /* Check for specific vulnerabilities */
    if (!vuln_name) return 0;
    
    /* Simple string matching for common vulnerabilities */
    if (errata_info.vendor_intel) {
        /* Check Intel-specific vulnerabilities */
        for (u32 i = 0; i < errata_info.active_errata_count; i++) {
            u16 id = errata_info.active_errata[i];
            if (id >= 1001 && id <= 1010) { /* Intel security errata range */
                return 1;
            }
        }
    }
    
    if (errata_info.vendor_amd) {
        /* Check AMD-specific vulnerabilities */
        for (u32 i = 0; i < errata_info.active_errata_count; i++) {
            u16 id = errata_info.active_errata[i];
            if (id >= 2001 && id <= 2010) { /* AMD security errata range */
                return 1;
            }
        }
    }
    
    return 0;
}
