/*
 * cache.c – x86 CPU cache management and control
 */

#include "kernel/types.h"

/* Cache control MSRs */
#define MSR_MTRRCAP             0xFE
#define MSR_MTRRDEFTYPE         0x2FF
#define MSR_MTRRPHYSBASE0       0x200
#define MSR_MTRRPHYSMASK0       0x201
#define MSR_MTRRFIX64K_00000    0x250
#define MSR_MTRRFIX16K_80000    0x258
#define MSR_MTRRFIX16K_A0000    0x259
#define MSR_MTRRFIX4K_C0000     0x268
#define MSR_MTRRFIX4K_C8000     0x269
#define MSR_MTRRFIX4K_D0000     0x26A
#define MSR_MTRRFIX4K_D8000     0x26B
#define MSR_MTRRFIX4K_E0000     0x26C
#define MSR_MTRRFIX4K_E8000     0x26D
#define MSR_MTRRFIX4K_F0000     0x26E
#define MSR_MTRRFIX4K_F8000     0x26F
#define MSR_PAT                 0x277

/* MTRR memory types */
#define MTRR_TYPE_UC            0x00  /* Uncacheable */
#define MTRR_TYPE_WC            0x01  /* Write Combining */
#define MTRR_TYPE_WT            0x04  /* Write Through */
#define MTRR_TYPE_WP            0x05  /* Write Protected */
#define MTRR_TYPE_WB            0x06  /* Write Back */

/* PAT memory types */
#define PAT_UC                  0x00  /* Uncacheable */
#define PAT_WC                  0x01  /* Write Combining */
#define PAT_WT                  0x04  /* Write Through */
#define PAT_WP                  0x05  /* Write Protected */
#define PAT_WB                  0x06  /* Write Back */
#define PAT_UCMINUS             0x07  /* Uncacheable Minus */

/* Cache control register bits */
#define CR0_CD                  0x40000000  /* Cache Disable */
#define CR0_NW                  0x20000000  /* Not Write-through */

typedef struct {
    u64 base;
    u64 mask;
    u8 type;
    u8 valid;
} mtrr_entry_t;

typedef struct {
    u8 mtrr_supported;
    u8 pat_supported;
    u8 wbinvd_supported;
    u8 clflush_supported;
    u8 num_variable_mtrrs;
    u8 num_fixed_mtrrs;
    mtrr_entry_t variable_mtrrs[16];
    u64 pat_value;
    u32 cache_line_size;
    u32 l1_cache_size;
    u32 l2_cache_size;
    u32 l3_cache_size;
} cache_info_t;

static cache_info_t cache_info;

extern u64 msr_read(u32 msr);
extern void msr_write(u32 msr, u64 value);
extern u8 msr_is_supported(void);

static u8 cache_check_mtrr_support(void) {
    u32 eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    
    return (edx & (1 << 12)) != 0;
}

static u8 cache_check_pat_support(void) {
    u32 eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    
    return (edx & (1 << 16)) != 0;
}

static u8 cache_check_clflush_support(void) {
    u32 eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    
    return (edx & (1 << 19)) != 0;
}

static void cache_detect_sizes(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check for cache size information */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x80000005));
    
    if (eax >= 0x80000005) {
        cache_info.l1_cache_size = (ecx >> 24) & 0xFF;
        cache_info.cache_line_size = ecx & 0xFF;
    }
    
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x80000006));
    
    if (eax >= 0x80000006) {
        cache_info.l2_cache_size = (ecx >> 16) & 0xFFFF;
        cache_info.l3_cache_size = ((edx >> 18) & 0x3FFF) * 512;
    }
    
    /* Fallback detection using CPUID leaf 4 */
    if (cache_info.cache_line_size == 0) {
        __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(4), "c"(0));
        
        if ((eax & 0x1F) != 0) {
            cache_info.cache_line_size = ((ebx & 0xFFF) + 1);
            u32 ways = ((ebx >> 22) & 0x3FF) + 1;
            u32 partitions = ((ebx >> 12) & 0x3FF) + 1;
            u32 sets = ecx + 1;
            cache_info.l1_cache_size = (ways * partitions * cache_info.cache_line_size * sets) / 1024;
        }
    }
}

static void cache_init_mtrrs(void) {
    if (!cache_info.mtrr_supported) return;
    
    u64 mtrrcap = msr_read(MSR_MTRRCAP);
    cache_info.num_variable_mtrrs = mtrrcap & 0xFF;
    cache_info.num_fixed_mtrrs = (mtrrcap & (1 << 8)) ? 11 : 0;
    
    /* Read existing MTRR settings */
    for (u8 i = 0; i < cache_info.num_variable_mtrrs && i < 16; i++) {
        u64 base = msr_read(MSR_MTRRPHYSBASE0 + i * 2);
        u64 mask = msr_read(MSR_MTRRPHYSMASK0 + i * 2);
        
        cache_info.variable_mtrrs[i].base = base & 0xFFFFF000;
        cache_info.variable_mtrrs[i].mask = mask & 0xFFFFF000;
        cache_info.variable_mtrrs[i].type = base & 0xFF;
        cache_info.variable_mtrrs[i].valid = (mask & (1 << 11)) != 0;
    }
}

static void cache_init_pat(void) {
    if (!cache_info.pat_supported) return;
    
    /* Set up default PAT values */
    cache_info.pat_value = 
        ((u64)PAT_WB << 0) |   /* PA0: Write Back */
        ((u64)PAT_WT << 8) |   /* PA1: Write Through */
        ((u64)PAT_UCMINUS << 16) | /* PA2: Uncacheable Minus */
        ((u64)PAT_UC << 24) |  /* PA3: Uncacheable */
        ((u64)PAT_WB << 32) |  /* PA4: Write Back */
        ((u64)PAT_WT << 40) |  /* PA5: Write Through */
        ((u64)PAT_UCMINUS << 48) | /* PA6: Uncacheable Minus */
        ((u64)PAT_WC << 56);   /* PA7: Write Combining */
    
    msr_write(MSR_PAT, cache_info.pat_value);
}

void cache_init(void) {
    if (!msr_is_supported()) return;
    
    cache_info.mtrr_supported = cache_check_mtrr_support();
    cache_info.pat_supported = cache_check_pat_support();
    cache_info.clflush_supported = cache_check_clflush_support();
    cache_info.wbinvd_supported = 1; /* Always available on x86 */
    
    cache_detect_sizes();
    cache_init_mtrrs();
    cache_init_pat();
}

void cache_flush_all(void) {
    if (cache_info.wbinvd_supported) {
        __asm__ volatile("wbinvd" : : : "memory");
    }
}

void cache_invalidate_all(void) {
    __asm__ volatile("invd" : : : "memory");
}

void cache_flush_line(void* address) {
    if (cache_info.clflush_supported) {
        __asm__ volatile("clflush (%0)" : : "r"(address) : "memory");
    }
}

void cache_enable(void) {
    u32 cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(CR0_CD | CR0_NW);
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
}

void cache_disable(void) {
    u32 cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= CR0_CD | CR0_NW;
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
    
    cache_flush_all();
}

u8 cache_setup_mtrr(u8 reg, u64 base, u64 size, u8 type) {
    if (!cache_info.mtrr_supported || reg >= cache_info.num_variable_mtrrs) {
        return 0;
    }
    
    /* Calculate mask from size */
    u64 mask = ~(size - 1) & 0xFFFFF000;
    mask |= (1 << 11); /* Valid bit */
    
    /* Disable MTRRs temporarily */
    u64 deftype = msr_read(MSR_MTRRDEFTYPE);
    msr_write(MSR_MTRRDEFTYPE, deftype & ~(1 << 11));
    
    /* Set MTRR */
    msr_write(MSR_MTRRPHYSBASE0 + reg * 2, (base & 0xFFFFF000) | type);
    msr_write(MSR_MTRRPHYSMASK0 + reg * 2, mask);
    
    /* Re-enable MTRRs */
    msr_write(MSR_MTRRDEFTYPE, deftype);
    
    /* Update local copy */
    cache_info.variable_mtrrs[reg].base = base & 0xFFFFF000;
    cache_info.variable_mtrrs[reg].mask = mask & 0xFFFFF000;
    cache_info.variable_mtrrs[reg].type = type;
    cache_info.variable_mtrrs[reg].valid = 1;
    
    cache_flush_all();
    return 1;
}

void cache_clear_mtrr(u8 reg) {
    if (!cache_info.mtrr_supported || reg >= cache_info.num_variable_mtrrs) {
        return;
    }
    
    /* Disable MTRRs temporarily */
    u64 deftype = msr_read(MSR_MTRRDEFTYPE);
    msr_write(MSR_MTRRDEFTYPE, deftype & ~(1 << 11));
    
    /* Clear MTRR */
    msr_write(MSR_MTRRPHYSBASE0 + reg * 2, 0);
    msr_write(MSR_MTRRPHYSMASK0 + reg * 2, 0);
    
    /* Re-enable MTRRs */
    msr_write(MSR_MTRRDEFTYPE, deftype);
    
    /* Update local copy */
    cache_info.variable_mtrrs[reg].valid = 0;
    
    cache_flush_all();
}

void cache_enable_mtrrs(void) {
    if (!cache_info.mtrr_supported) return;
    
    u64 deftype = msr_read(MSR_MTRRDEFTYPE);
    deftype |= (1 << 11); /* Enable MTRRs */
    deftype |= MTRR_TYPE_WB; /* Default type: Write Back */
    msr_write(MSR_MTRRDEFTYPE, deftype);
    
    cache_flush_all();
}

void cache_disable_mtrrs(void) {
    if (!cache_info.mtrr_supported) return;
    
    u64 deftype = msr_read(MSR_MTRRDEFTYPE);
    deftype &= ~(1 << 11); /* Disable MTRRs */
    msr_write(MSR_MTRRDEFTYPE, deftype);
    
    cache_flush_all();
}

u8 cache_is_mtrr_supported(void) {
    return cache_info.mtrr_supported;
}

u8 cache_is_pat_supported(void) {
    return cache_info.pat_supported;
}

u8 cache_get_num_variable_mtrrs(void) {
    return cache_info.num_variable_mtrrs;
}

u32 cache_get_line_size(void) {
    return cache_info.cache_line_size;
}

u32 cache_get_l1_size(void) {
    return cache_info.l1_cache_size;
}

u32 cache_get_l2_size(void) {
    return cache_info.l2_cache_size;
}

u32 cache_get_l3_size(void) {
    return cache_info.l3_cache_size;
}

void cache_prefetch(void* address) {
    __asm__ volatile("prefetcht0 (%0)" : : "r"(address));
}

void cache_prefetch_nta(void* address) {
    __asm__ volatile("prefetchnta (%0)" : : "r"(address));
}

u8 cache_get_mtrr_type(u8 reg) {
    if (reg >= cache_info.num_variable_mtrrs) return 0;
    return cache_info.variable_mtrrs[reg].type;
}

u64 cache_get_mtrr_base(u8 reg) {
    if (reg >= cache_info.num_variable_mtrrs) return 0;
    return cache_info.variable_mtrrs[reg].base;
}

u64 cache_get_mtrr_mask(u8 reg) {
    if (reg >= cache_info.num_variable_mtrrs) return 0;
    return cache_info.variable_mtrrs[reg].mask;
}

u8 cache_is_mtrr_valid(u8 reg) {
    if (reg >= cache_info.num_variable_mtrrs) return 0;
    return cache_info.variable_mtrrs[reg].valid;
}
