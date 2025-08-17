/*
 * memory_mgmt.c – x86 advanced memory management (PAT/MTRR extensions)
 */

#include "kernel/types.h"

/* Memory type definitions */
#define MEMORY_TYPE_UC          0   /* Uncacheable */
#define MEMORY_TYPE_WC          1   /* Write Combining */
#define MEMORY_TYPE_WT          4   /* Write Through */
#define MEMORY_TYPE_WP          5   /* Write Protected */
#define MEMORY_TYPE_WB          6   /* Write Back */
#define MEMORY_TYPE_UC_MINUS    7   /* Uncacheable Minus */

/* PAT MSR */
#define MSR_IA32_PAT            0x277

/* MTRR MSRs */
#define MSR_IA32_MTRRCAP        0xFE
#define MSR_IA32_MTRR_DEF_TYPE  0x2FF
#define MSR_IA32_MTRR_FIX64K    0x250
#define MSR_IA32_MTRR_FIX16K_80000 0x258
#define MSR_IA32_MTRR_FIX16K_A0000 0x259
#define MSR_IA32_MTRR_FIX4K_C0000  0x268
#define MSR_IA32_MTRR_FIX4K_F8000  0x26F

/* Page attribute bits */
#define PAGE_ATTR_PAT           0x080
#define PAGE_ATTR_PCD           0x010
#define PAGE_ATTR_PWT           0x008

/* Memory range types */
#define RANGE_TYPE_FIXED        0
#define RANGE_TYPE_VARIABLE     1

typedef struct {
    u64 base;
    u64 size;
    u8 type;
    u8 valid;
} memory_range_t;

typedef struct {
    u8 memory_mgmt_supported;
    u8 pat_supported;
    u8 mtrr_supported;
    u8 smrr_supported;
    u8 num_variable_mtrrs;
    u8 fixed_mtrrs_supported;
    u8 wc_supported;
    u64 pat_value;
    u64 default_memory_type;
    memory_range_t variable_ranges[16];
    memory_range_t fixed_ranges[88];
    u8 memory_encryption_supported;
    u8 memory_protection_keys_supported;
} memory_mgmt_info_t;

static memory_mgmt_info_t memory_mgmt_info;

extern u64 msr_read(u32 msr);
extern void msr_write(u32 msr, u64 value);
extern u8 msr_is_supported(void);

static void memory_mgmt_detect_capabilities(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check for PAT support */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    memory_mgmt_info.pat_supported = (edx & (1 << 16)) != 0;
    
    /* Check for MTRR support */
    memory_mgmt_info.mtrr_supported = (edx & (1 << 12)) != 0;
    
    if (memory_mgmt_info.mtrr_supported && msr_is_supported()) {
        /* Read MTRR capabilities */
        u64 mtrrcap = msr_read(MSR_IA32_MTRRCAP);
        memory_mgmt_info.num_variable_mtrrs = mtrrcap & 0xFF;
        memory_mgmt_info.fixed_mtrrs_supported = (mtrrcap & (1 << 8)) != 0;
        memory_mgmt_info.wc_supported = (mtrrcap & (1 << 10)) != 0;
        memory_mgmt_info.smrr_supported = (mtrrcap & (1 << 11)) != 0;
    }
    
    /* Check for memory encryption */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x80000000));
    if (eax >= 0x8000001F) {
        __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x8000001F));
        memory_mgmt_info.memory_encryption_supported = (eax & 1) != 0;
    }
    
    /* Check for protection keys */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    memory_mgmt_info.memory_protection_keys_supported = (ecx & (1 << 3)) != 0;
}

static void memory_mgmt_setup_pat(void) {
    if (!memory_mgmt_info.pat_supported || !msr_is_supported()) return;
    
    /* Setup default PAT entries */
    u64 pat = 0;
    pat |= ((u64)MEMORY_TYPE_WB << 0);   /* PAT0: Write Back */
    pat |= ((u64)MEMORY_TYPE_WT << 8);   /* PAT1: Write Through */
    pat |= ((u64)MEMORY_TYPE_UC_MINUS << 16); /* PAT2: Uncacheable Minus */
    pat |= ((u64)MEMORY_TYPE_UC << 24);  /* PAT3: Uncacheable */
    pat |= ((u64)MEMORY_TYPE_WB << 32);  /* PAT4: Write Back */
    pat |= ((u64)MEMORY_TYPE_WT << 40);  /* PAT5: Write Through */
    pat |= ((u64)MEMORY_TYPE_UC_MINUS << 48); /* PAT6: Uncacheable Minus */
    pat |= ((u64)MEMORY_TYPE_WC << 56);  /* PAT7: Write Combining */
    
    msr_write(MSR_IA32_PAT, pat);
    memory_mgmt_info.pat_value = pat;
}

static void memory_mgmt_read_mtrr_ranges(void) {
    if (!memory_mgmt_info.mtrr_supported || !msr_is_supported()) return;
    
    /* Read default memory type */
    u64 def_type = msr_read(MSR_IA32_MTRR_DEF_TYPE);
    memory_mgmt_info.default_memory_type = def_type & 0xFF;
    
    /* Read variable MTRRs */
    for (u8 i = 0; i < memory_mgmt_info.num_variable_mtrrs && i < 16; i++) {
        u64 base = msr_read(0x200 + i * 2);
        u64 mask = msr_read(0x201 + i * 2);
        
        memory_mgmt_info.variable_ranges[i].valid = (mask & (1ULL << 11)) != 0;
        if (memory_mgmt_info.variable_ranges[i].valid) {
            memory_mgmt_info.variable_ranges[i].base = base & 0xFFFFFFFFFFFFF000ULL;
            memory_mgmt_info.variable_ranges[i].type = base & 0xFF;
            
            /* Calculate size from mask */
            u64 size_mask = mask & 0xFFFFFFFFFFFFF000ULL;
            memory_mgmt_info.variable_ranges[i].size = ~size_mask + 1;
        }
    }
    
    /* Read fixed MTRRs if supported */
    if (memory_mgmt_info.fixed_mtrrs_supported) {
        /* 64K fixed ranges */
        u64 fix64k = msr_read(MSR_IA32_MTRR_FIX64K);
        for (u8 i = 0; i < 8; i++) {
            memory_mgmt_info.fixed_ranges[i].base = i * 0x10000;
            memory_mgmt_info.fixed_ranges[i].size = 0x10000;
            memory_mgmt_info.fixed_ranges[i].type = (fix64k >> (i * 8)) & 0xFF;
            memory_mgmt_info.fixed_ranges[i].valid = 1;
        }
        
        /* 16K fixed ranges */
        u64 fix16k_80000 = msr_read(MSR_IA32_MTRR_FIX16K_80000);
        u64 fix16k_a0000 = msr_read(MSR_IA32_MTRR_FIX16K_A0000);
        
        for (u8 i = 0; i < 8; i++) {
            memory_mgmt_info.fixed_ranges[8 + i].base = 0x80000 + i * 0x4000;
            memory_mgmt_info.fixed_ranges[8 + i].size = 0x4000;
            memory_mgmt_info.fixed_ranges[8 + i].type = (fix16k_80000 >> (i * 8)) & 0xFF;
            memory_mgmt_info.fixed_ranges[8 + i].valid = 1;
            
            memory_mgmt_info.fixed_ranges[16 + i].base = 0xA0000 + i * 0x4000;
            memory_mgmt_info.fixed_ranges[16 + i].size = 0x4000;
            memory_mgmt_info.fixed_ranges[16 + i].type = (fix16k_a0000 >> (i * 8)) & 0xFF;
            memory_mgmt_info.fixed_ranges[16 + i].valid = 1;
        }
        
        /* 4K fixed ranges */
        for (u8 msr_idx = 0; msr_idx < 8; msr_idx++) {
            u64 fix4k = msr_read(MSR_IA32_MTRR_FIX4K_C0000 + msr_idx);
            
            for (u8 i = 0; i < 8; i++) {
                u8 range_idx = 24 + msr_idx * 8 + i;
                memory_mgmt_info.fixed_ranges[range_idx].base = 0xC0000 + (msr_idx * 8 + i) * 0x1000;
                memory_mgmt_info.fixed_ranges[range_idx].size = 0x1000;
                memory_mgmt_info.fixed_ranges[range_idx].type = (fix4k >> (i * 8)) & 0xFF;
                memory_mgmt_info.fixed_ranges[range_idx].valid = 1;
            }
        }
    }
}

void memory_mgmt_init(void) {
    memory_mgmt_info.memory_mgmt_supported = 0;
    memory_mgmt_info.pat_supported = 0;
    memory_mgmt_info.mtrr_supported = 0;
    memory_mgmt_info.smrr_supported = 0;
    memory_mgmt_info.num_variable_mtrrs = 0;
    memory_mgmt_info.fixed_mtrrs_supported = 0;
    memory_mgmt_info.wc_supported = 0;
    memory_mgmt_info.memory_encryption_supported = 0;
    memory_mgmt_info.memory_protection_keys_supported = 0;
    
    memory_mgmt_detect_capabilities();
    
    if (memory_mgmt_info.pat_supported || memory_mgmt_info.mtrr_supported) {
        memory_mgmt_info.memory_mgmt_supported = 1;
        
        memory_mgmt_setup_pat();
        memory_mgmt_read_mtrr_ranges();
    }
}

u8 memory_mgmt_is_supported(void) {
    return memory_mgmt_info.memory_mgmt_supported;
}

u8 memory_mgmt_is_pat_supported(void) {
    return memory_mgmt_info.pat_supported;
}

u8 memory_mgmt_is_mtrr_supported(void) {
    return memory_mgmt_info.mtrr_supported;
}

u8 memory_mgmt_is_wc_supported(void) {
    return memory_mgmt_info.wc_supported;
}

u8 memory_mgmt_get_num_variable_mtrrs(void) {
    return memory_mgmt_info.num_variable_mtrrs;
}

u8 memory_mgmt_is_fixed_mtrrs_supported(void) {
    return memory_mgmt_info.fixed_mtrrs_supported;
}

u64 memory_mgmt_get_pat_value(void) {
    return memory_mgmt_info.pat_value;
}

u8 memory_mgmt_get_default_memory_type(void) {
    return memory_mgmt_info.default_memory_type;
}

u8 memory_mgmt_set_variable_mtrr(u8 index, u64 base, u64 size, u8 type) {
    if (!memory_mgmt_info.mtrr_supported || !msr_is_supported()) return 0;
    if (index >= memory_mgmt_info.num_variable_mtrrs) return 0;
    
    /* Disable MTRRs during update */
    u64 def_type = msr_read(MSR_IA32_MTRR_DEF_TYPE);
    msr_write(MSR_IA32_MTRR_DEF_TYPE, def_type & ~(1ULL << 11));
    
    /* Set base and type */
    msr_write(0x200 + index * 2, (base & 0xFFFFFFFFFFFFF000ULL) | type);
    
    /* Set mask and enable */
    u64 mask = ~(size - 1) & 0xFFFFFFFFFFFFF000ULL;
    msr_write(0x201 + index * 2, mask | (1ULL << 11));
    
    /* Re-enable MTRRs */
    msr_write(MSR_IA32_MTRR_DEF_TYPE, def_type);
    
    /* Update local copy */
    memory_mgmt_info.variable_ranges[index].base = base;
    memory_mgmt_info.variable_ranges[index].size = size;
    memory_mgmt_info.variable_ranges[index].type = type;
    memory_mgmt_info.variable_ranges[index].valid = 1;
    
    return 1;
}

u8 memory_mgmt_clear_variable_mtrr(u8 index) {
    if (!memory_mgmt_info.mtrr_supported || !msr_is_supported()) return 0;
    if (index >= memory_mgmt_info.num_variable_mtrrs) return 0;
    
    /* Disable MTRRs during update */
    u64 def_type = msr_read(MSR_IA32_MTRR_DEF_TYPE);
    msr_write(MSR_IA32_MTRR_DEF_TYPE, def_type & ~(1ULL << 11));
    
    /* Clear MTRR */
    msr_write(0x200 + index * 2, 0);
    msr_write(0x201 + index * 2, 0);
    
    /* Re-enable MTRRs */
    msr_write(MSR_IA32_MTRR_DEF_TYPE, def_type);
    
    /* Update local copy */
    memory_mgmt_info.variable_ranges[index].valid = 0;
    
    return 1;
}

const memory_range_t* memory_mgmt_get_variable_range(u8 index) {
    if (index >= memory_mgmt_info.num_variable_mtrrs) return 0;
    return &memory_mgmt_info.variable_ranges[index];
}

const memory_range_t* memory_mgmt_get_fixed_range(u8 index) {
    if (index >= 88) return 0;
    return &memory_mgmt_info.fixed_ranges[index];
}

u8 memory_mgmt_get_memory_type(u64 address) {
    /* Check variable MTRRs first */
    for (u8 i = 0; i < memory_mgmt_info.num_variable_mtrrs; i++) {
        const memory_range_t* range = &memory_mgmt_info.variable_ranges[i];
        if (range->valid && address >= range->base && 
            address < range->base + range->size) {
            return range->type;
        }
    }
    
    /* Check fixed MTRRs for addresses below 1MB */
    if (address < 0x100000 && memory_mgmt_info.fixed_mtrrs_supported) {
        for (u8 i = 0; i < 88; i++) {
            const memory_range_t* range = &memory_mgmt_info.fixed_ranges[i];
            if (range->valid && address >= range->base && 
                address < range->base + range->size) {
                return range->type;
            }
        }
    }
    
    /* Return default type */
    return memory_mgmt_info.default_memory_type;
}

u32 memory_mgmt_get_page_attributes(u8 memory_type) {
    u32 attributes = 0;
    
    switch (memory_type) {
        case MEMORY_TYPE_UC:
            attributes = PAGE_ATTR_PCD | PAGE_ATTR_PWT;
            break;
        case MEMORY_TYPE_WC:
            attributes = PAGE_ATTR_PAT | PAGE_ATTR_PWT;
            break;
        case MEMORY_TYPE_WT:
            attributes = PAGE_ATTR_PWT;
            break;
        case MEMORY_TYPE_WP:
            attributes = PAGE_ATTR_PAT | PAGE_ATTR_PCD;
            break;
        case MEMORY_TYPE_WB:
            attributes = 0; /* Default */
            break;
        case MEMORY_TYPE_UC_MINUS:
            attributes = PAGE_ATTR_PCD;
            break;
    }
    
    return attributes;
}

void memory_mgmt_flush_tlb(void) {
    u32 cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    __asm__ volatile("mov %0, %%cr3" : : "r"(cr3) : "memory");
}

void memory_mgmt_flush_tlb_single(u64 address) {
    __asm__ volatile("invlpg (%0)" : : "r"(address) : "memory");
}

u8 memory_mgmt_is_memory_encryption_supported(void) {
    return memory_mgmt_info.memory_encryption_supported;
}

u8 memory_mgmt_is_protection_keys_supported(void) {
    return memory_mgmt_info.memory_protection_keys_supported;
}

u8 memory_mgmt_is_smrr_supported(void) {
    return memory_mgmt_info.smrr_supported;
}

void memory_mgmt_enable_mtrrs(void) {
    if (!memory_mgmt_info.mtrr_supported || !msr_is_supported()) return;
    
    u64 def_type = msr_read(MSR_IA32_MTRR_DEF_TYPE);
    def_type |= (1ULL << 11); /* Enable MTRRs */
    msr_write(MSR_IA32_MTRR_DEF_TYPE, def_type);
}

void memory_mgmt_disable_mtrrs(void) {
    if (!memory_mgmt_info.mtrr_supported || !msr_is_supported()) return;
    
    u64 def_type = msr_read(MSR_IA32_MTRR_DEF_TYPE);
    def_type &= ~(1ULL << 11); /* Disable MTRRs */
    msr_write(MSR_IA32_MTRR_DEF_TYPE, def_type);
}
