/*
 * microcode.c – x86 CPU microcode update support
 */

#include "kernel/types.h"

/* Microcode MSRs */
#define MSR_IA32_PLATFORM_ID    0x17
#define MSR_IA32_BIOS_UPDT_TRIG 0x79
#define MSR_IA32_BIOS_SIGN_ID   0x8B

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
} __attribute__((packed)) intel_microcode_header_t;

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
} __attribute__((packed)) amd_microcode_header_t;

typedef struct {
    u8 vendor_intel;
    u8 vendor_amd;
    u32 current_revision;
    u32 processor_signature;
    u32 processor_flags;
    u32 platform_id;
    u8 update_available;
    u32 update_revision;
    const void* update_data;
    u32 update_size;
} microcode_info_t;

static microcode_info_t microcode_info;

extern u64 msr_read(u32 msr);
extern void msr_write(u32 msr, u64 value);
extern u8 msr_is_supported(void);

static void microcode_detect_vendor(void) {
    u32 eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0));
    
    /* Check for Intel */
    if (ebx == 0x756E6547 && edx == 0x49656E69 && ecx == 0x6C65746E) {
        microcode_info.vendor_intel = 1;
        microcode_info.vendor_amd = 0;
    }
    /* Check for AMD */
    else if (ebx == 0x68747541 && edx == 0x69746E65 && ecx == 0x444D4163) {
        microcode_info.vendor_intel = 0;
        microcode_info.vendor_amd = 1;
    }
    else {
        microcode_info.vendor_intel = 0;
        microcode_info.vendor_amd = 0;
    }
}

static void microcode_get_processor_info(void) {
    u32 eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    
    microcode_info.processor_signature = eax;
    
    if (microcode_info.vendor_intel) {
        /* Get platform ID for Intel */
        u64 platform_id = msr_read(MSR_IA32_PLATFORM_ID);
        microcode_info.platform_id = (platform_id >> 50) & 0x7;
        microcode_info.processor_flags = 1 << microcode_info.platform_id;
    } else if (microcode_info.vendor_amd) {
        /* AMD doesn't use platform ID */
        microcode_info.platform_id = 0;
        microcode_info.processor_flags = 0;
    }
}

static u32 microcode_get_current_revision(void) {
    if (microcode_info.vendor_intel) {
        /* Intel microcode revision */
        msr_write(MSR_IA32_BIOS_SIGN_ID, 0);
        
        /* Execute CPUID to load revision */
        u32 eax;
        __asm__ volatile("cpuid" : "=a"(eax) : "a"(1) : "ebx", "ecx", "edx");
        
        u64 bios_sign_id = msr_read(MSR_IA32_BIOS_SIGN_ID);
        return (u32)(bios_sign_id >> 32);
    } else if (microcode_info.vendor_amd) {
        /* AMD microcode revision */
        u64 patch_level = msr_read(0x0000008B);
        return (u32)patch_level;
    }
    
    return 0;
}

static u32 microcode_calculate_checksum(const void* data, u32 size) {
    const u32* words = (const u32*)data;
    u32 checksum = 0;
    
    for (u32 i = 0; i < size / 4; i++) {
        checksum += words[i];
    }
    
    return checksum;
}

static u8 microcode_validate_intel_update(const intel_microcode_header_t* header) {
    /* Check header version */
    if (header->header_version != 0x00000001) {
        return 0;
    }
    
    /* Check processor signature */
    if (header->processor_signature != microcode_info.processor_signature) {
        return 0;
    }
    
    /* Check processor flags */
    if (!(header->processor_flags & microcode_info.processor_flags)) {
        return 0;
    }
    
    /* Check data size */
    if (header->data_size == 0) {
        header = (const intel_microcode_header_t*)((const u8*)header);
        if (header->total_size < 2048 || header->total_size > 32768) {
            return 0;
        }
    } else {
        if (header->data_size < 2000 || header->data_size > 32000) {
            return 0;
        }
    }
    
    /* Verify checksum */
    u32 size = header->total_size ? header->total_size : 2048;
    u32 checksum = microcode_calculate_checksum(header, size);
    
    return checksum == 0;
}

static u8 microcode_validate_amd_update(const amd_microcode_header_t* header) {
    /* Check data code */
    if (header->data_code != 0x00000001) {
        return 0;
    }
    
    /* Check processor revision */
    u32 family = (microcode_info.processor_signature >> 8) & 0xF;
    u32 model = (microcode_info.processor_signature >> 4) & 0xF;
    u32 stepping = microcode_info.processor_signature & 0xF;
    
    if (family >= 0xF) {
        family += ((microcode_info.processor_signature >> 20) & 0xFF);
    }
    if (family >= 0xF || (family == 0x6 && model >= 0xF)) {
        model += ((microcode_info.processor_signature >> 16) & 0xF) << 4;
    }
    
    u16 expected_rev = (family << 12) | (model << 4) | stepping;
    
    return header->processor_rev_id == expected_rev;
}

static u8 microcode_apply_intel_update(const intel_microcode_header_t* header) {
    /* Disable interrupts */
    __asm__ volatile("cli");
    
    /* Write microcode data to MSR */
    u32 data_addr = (u32)header + sizeof(intel_microcode_header_t);
    msr_write(MSR_IA32_BIOS_UPDT_TRIG, data_addr);
    
    /* Re-enable interrupts */
    __asm__ volatile("sti");
    
    /* Verify update was applied */
    u32 new_revision = microcode_get_current_revision();
    
    return new_revision == header->update_revision;
}

static u8 microcode_apply_amd_update(const amd_microcode_header_t* header) {
    /* AMD microcode updates are typically handled by BIOS */
    /* This is a simplified implementation */
    
    u32 patch_addr = (u32)header;
    
    /* Write patch address to MSR */
    msr_write(0xC0010020, patch_addr);
    
    /* Verify update */
    u32 new_revision = microcode_get_current_revision();
    
    return new_revision == header->patch_id;
}

void microcode_init(void) {
    if (!msr_is_supported()) return;
    
    microcode_detect_vendor();
    
    if (!microcode_info.vendor_intel && !microcode_info.vendor_amd) {
        return;
    }
    
    microcode_get_processor_info();
    microcode_info.current_revision = microcode_get_current_revision();
    microcode_info.update_available = 0;
    microcode_info.update_data = 0;
    microcode_info.update_size = 0;
}

u8 microcode_is_supported(void) {
    return microcode_info.vendor_intel || microcode_info.vendor_amd;
}

u8 microcode_is_intel(void) {
    return microcode_info.vendor_intel;
}

u8 microcode_is_amd(void) {
    return microcode_info.vendor_amd;
}

u32 microcode_get_revision(void) {
    return microcode_info.current_revision;
}

u32 microcode_get_processor_signature(void) {
    return microcode_info.processor_signature;
}

u32 microcode_get_processor_flags(void) {
    return microcode_info.processor_flags;
}

u8 microcode_load_update(const void* update_data, u32 size) {
    if (!microcode_is_supported() || !update_data || size < 48) {
        return 0;
    }
    
    if (microcode_info.vendor_intel) {
        const intel_microcode_header_t* header = (const intel_microcode_header_t*)update_data;
        
        if (!microcode_validate_intel_update(header)) {
            return 0;
        }
        
        /* Check if this is a newer revision */
        if (header->update_revision <= microcode_info.current_revision) {
            return 0;
        }
        
        microcode_info.update_data = update_data;
        microcode_info.update_size = size;
        microcode_info.update_revision = header->update_revision;
        microcode_info.update_available = 1;
        
        return 1;
    } else if (microcode_info.vendor_amd) {
        const amd_microcode_header_t* header = (const amd_microcode_header_t*)update_data;
        
        if (!microcode_validate_amd_update(header)) {
            return 0;
        }
        
        /* Check if this is a newer revision */
        if (header->patch_id <= microcode_info.current_revision) {
            return 0;
        }
        
        microcode_info.update_data = update_data;
        microcode_info.update_size = size;
        microcode_info.update_revision = header->patch_id;
        microcode_info.update_available = 1;
        
        return 1;
    }
    
    return 0;
}

u8 microcode_apply_update(void) {
    if (!microcode_info.update_available || !microcode_info.update_data) {
        return 0;
    }
    
    u8 success = 0;
    
    if (microcode_info.vendor_intel) {
        const intel_microcode_header_t* header = 
            (const intel_microcode_header_t*)microcode_info.update_data;
        success = microcode_apply_intel_update(header);
    } else if (microcode_info.vendor_amd) {
        const amd_microcode_header_t* header = 
            (const amd_microcode_header_t*)microcode_info.update_data;
        success = microcode_apply_amd_update(header);
    }
    
    if (success) {
        microcode_info.current_revision = microcode_info.update_revision;
        microcode_info.update_available = 0;
    }
    
    return success;
}

u8 microcode_is_update_available(void) {
    return microcode_info.update_available;
}

u32 microcode_get_update_revision(void) {
    return microcode_info.update_revision;
}

void microcode_refresh_revision(void) {
    microcode_info.current_revision = microcode_get_current_revision();
}
