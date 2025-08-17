/*
 * cache_mgmt.c – x86 CPU cache management (L1/L2/L3 control)
 */

#include "kernel/types.h"

/* Cache control MSRs */
#define MSR_IA32_MTRRCAP        0xFE
#define MSR_IA32_MTRR_DEF_TYPE  0x2FF
#define MSR_IA32_MTRR_PHYSBASE0 0x200
#define MSR_IA32_MTRR_PHYSMASK0 0x201
#define MSR_IA32_PAT            0x277

/* Cache control instructions */
#define CACHE_FLUSH_ALL         0
#define CACHE_FLUSH_L1D         1
#define CACHE_FLUSH_L1I         2
#define CACHE_FLUSH_L2          3
#define CACHE_FLUSH_L3          4

/* Cache types */
#define CACHE_TYPE_DATA         1
#define CACHE_TYPE_INSTRUCTION  2
#define CACHE_TYPE_UNIFIED      3

/* Cache policies */
#define CACHE_POLICY_WRITEBACK      0
#define CACHE_POLICY_WRITETHROUGH   1
#define CACHE_POLICY_WRITECOMBINING 2
#define CACHE_POLICY_WRITEPROTECT   3
#define CACHE_POLICY_UNCACHEABLE    4

typedef struct {
    u8 level;
    u8 type;
    u32 size;
    u16 line_size;
    u16 associativity;
    u32 sets;
    u8 inclusive;
    u8 complex_indexing;
    u8 prefetch_size;
    u8 shared_cores;
} cache_descriptor_t;

typedef struct {
    u8 cache_management_supported;
    u8 num_cache_levels;
    cache_descriptor_t caches[8];
    u8 l1d_flush_supported;
    u8 l1i_flush_supported;
    u8 l2_flush_supported;
    u8 l3_flush_supported;
    u8 cache_allocation_supported;
    u8 cache_monitoring_supported;
    u32 cache_line_size;
    u8 clflush_supported;
    u8 clflushopt_supported;
    u8 clwb_supported;
    u8 prefetch_control_supported;
} cache_mgmt_info_t;

static cache_mgmt_info_t cache_mgmt_info;

extern u64 msr_read(u32 msr);
extern void msr_write(u32 msr, u64 value);
extern u8 msr_is_supported(void);

static void cache_mgmt_detect_capabilities(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check for cache management features */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    
    cache_mgmt_info.clflush_supported = (edx & (1 << 19)) != 0;
    cache_mgmt_info.cache_line_size = ((ebx >> 8) & 0xFF) * 8;
    
    /* Check for extended cache features */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    
    cache_mgmt_info.clflushopt_supported = (ebx & (1 << 23)) != 0;
    cache_mgmt_info.clwb_supported = (ebx & (1 << 24)) != 0;
    
    /* Check for cache allocation technology */
    if (eax >= 0x10) {
        __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x10), "c"(0));
        cache_mgmt_info.cache_allocation_supported = (ebx & (1 << 1)) != 0;
    }
    
    /* Check for cache monitoring */
    if (eax >= 0xF) {
        __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0xF), "c"(0));
        cache_mgmt_info.cache_monitoring_supported = (edx & (1 << 1)) != 0;
    }
}

static void cache_mgmt_enumerate_caches(void) {
    u32 eax, ebx, ecx, edx;
    cache_mgmt_info.num_cache_levels = 0;
    
    /* Enumerate cache hierarchy using CPUID leaf 4 */
    for (u32 i = 0; i < 8; i++) {
        __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(4), "c"(i));
        
        u8 cache_type = eax & 0x1F;
        if (cache_type == 0) break; /* No more cache levels */
        
        cache_descriptor_t* cache = &cache_mgmt_info.caches[cache_mgmt_info.num_cache_levels];
        
        cache->level = (eax >> 5) & 0x7;
        cache->type = cache_type;
        cache->line_size = (ebx & 0xFFF) + 1;
        cache->associativity = ((ebx >> 22) & 0x3FF) + 1;
        cache->sets = ecx + 1;
        cache->size = cache->line_size * cache->associativity * cache->sets;
        cache->inclusive = (edx & (1 << 1)) != 0;
        cache->complex_indexing = (edx & (1 << 2)) != 0;
        cache->shared_cores = ((eax >> 14) & 0xFFF) + 1;
        
        /* Detect prefetch capabilities */
        if (cache->level == 1) {
            cache->prefetch_size = 64; /* Typical L1 prefetch */
        } else if (cache->level == 2) {
            cache->prefetch_size = 128; /* Typical L2 prefetch */
        } else if (cache->level == 3) {
            cache->prefetch_size = 256; /* Typical L3 prefetch */
        }
        
        cache_mgmt_info.num_cache_levels++;
    }
    
    /* Detect flush capabilities based on cache levels */
    for (u8 i = 0; i < cache_mgmt_info.num_cache_levels; i++) {
        cache_descriptor_t* cache = &cache_mgmt_info.caches[i];
        
        if (cache->level == 1 && cache->type == CACHE_TYPE_DATA) {
            cache_mgmt_info.l1d_flush_supported = 1;
        }
        if (cache->level == 1 && cache->type == CACHE_TYPE_INSTRUCTION) {
            cache_mgmt_info.l1i_flush_supported = 1;
        }
        if (cache->level == 2) {
            cache_mgmt_info.l2_flush_supported = 1;
        }
        if (cache->level == 3) {
            cache_mgmt_info.l3_flush_supported = 1;
        }
    }
}

void cache_mgmt_init(void) {
    cache_mgmt_info.cache_management_supported = 0;
    cache_mgmt_info.num_cache_levels = 0;
    cache_mgmt_info.l1d_flush_supported = 0;
    cache_mgmt_info.l1i_flush_supported = 0;
    cache_mgmt_info.l2_flush_supported = 0;
    cache_mgmt_info.l3_flush_supported = 0;
    cache_mgmt_info.cache_allocation_supported = 0;
    cache_mgmt_info.cache_monitoring_supported = 0;
    cache_mgmt_info.clflush_supported = 0;
    cache_mgmt_info.clflushopt_supported = 0;
    cache_mgmt_info.clwb_supported = 0;
    cache_mgmt_info.prefetch_control_supported = 0;
    
    cache_mgmt_detect_capabilities();
    cache_mgmt_enumerate_caches();
    
    if (cache_mgmt_info.num_cache_levels > 0) {
        cache_mgmt_info.cache_management_supported = 1;
    }
}

u8 cache_mgmt_is_supported(void) {
    return cache_mgmt_info.cache_management_supported;
}

u8 cache_mgmt_get_num_levels(void) {
    return cache_mgmt_info.num_cache_levels;
}

const cache_descriptor_t* cache_mgmt_get_cache_info(u8 level, u8 type) {
    for (u8 i = 0; i < cache_mgmt_info.num_cache_levels; i++) {
        cache_descriptor_t* cache = &cache_mgmt_info.caches[i];
        if (cache->level == level && (type == 0 || cache->type == type)) {
            return cache;
        }
    }
    return 0;
}

u32 cache_mgmt_get_cache_size(u8 level, u8 type) {
    const cache_descriptor_t* cache = cache_mgmt_get_cache_info(level, type);
    return cache ? cache->size : 0;
}

u16 cache_mgmt_get_line_size(u8 level, u8 type) {
    const cache_descriptor_t* cache = cache_mgmt_get_cache_info(level, type);
    return cache ? cache->line_size : cache_mgmt_info.cache_line_size;
}

u16 cache_mgmt_get_associativity(u8 level, u8 type) {
    const cache_descriptor_t* cache = cache_mgmt_get_cache_info(level, type);
    return cache ? cache->associativity : 0;
}

void cache_mgmt_flush_all(void) {
    __asm__ volatile("wbinvd" : : : "memory");
}

void cache_mgmt_flush_line(void* address) {
    if (!cache_mgmt_info.clflush_supported) return;
    
    if (cache_mgmt_info.clflushopt_supported) {
        __asm__ volatile("clflushopt (%0)" : : "r"(address) : "memory");
    } else {
        __asm__ volatile("clflush (%0)" : : "r"(address) : "memory");
    }
}

void cache_mgmt_writeback_line(void* address) {
    if (!cache_mgmt_info.clwb_supported) {
        cache_mgmt_flush_line(address);
        return;
    }
    
    __asm__ volatile("clwb (%0)" : : "r"(address) : "memory");
}

void cache_mgmt_flush_range(void* start, u32 size) {
    if (!cache_mgmt_info.clflush_supported) return;
    
    u32 line_size = cache_mgmt_info.cache_line_size;
    u8* addr = (u8*)((u32)start & ~(line_size - 1));
    u8* end = (u8*)start + size;
    
    while (addr < end) {
        cache_mgmt_flush_line(addr);
        addr += line_size;
    }
    
    __asm__ volatile("mfence" : : : "memory");
}

void cache_mgmt_writeback_range(void* start, u32 size) {
    u32 line_size = cache_mgmt_info.cache_line_size;
    u8* addr = (u8*)((u32)start & ~(line_size - 1));
    u8* end = (u8*)start + size;
    
    while (addr < end) {
        cache_mgmt_writeback_line(addr);
        addr += line_size;
    }
    
    __asm__ volatile("sfence" : : : "memory");
}

void cache_mgmt_prefetch(void* address, u8 locality) {
    /* Use prefetch instructions based on locality */
    switch (locality) {
        case 0: /* Non-temporal */
            __asm__ volatile("prefetchnta (%0)" : : "r"(address));
            break;
        case 1: /* Low temporal locality */
            __asm__ volatile("prefetcht2 (%0)" : : "r"(address));
            break;
        case 2: /* Moderate temporal locality */
            __asm__ volatile("prefetcht1 (%0)" : : "r"(address));
            break;
        case 3: /* High temporal locality */
            __asm__ volatile("prefetcht0 (%0)" : : "r"(address));
            break;
    }
}

void cache_mgmt_prefetch_range(void* start, u32 size, u8 locality) {
    u32 line_size = cache_mgmt_info.cache_line_size;
    u8* addr = (u8*)((u32)start & ~(line_size - 1));
    u8* end = (u8*)start + size;
    
    while (addr < end) {
        cache_mgmt_prefetch(addr, locality);
        addr += line_size;
    }
}

u8 cache_mgmt_is_clflush_supported(void) {
    return cache_mgmt_info.clflush_supported;
}

u8 cache_mgmt_is_clflushopt_supported(void) {
    return cache_mgmt_info.clflushopt_supported;
}

u8 cache_mgmt_is_clwb_supported(void) {
    return cache_mgmt_info.clwb_supported;
}

u8 cache_mgmt_is_cache_allocation_supported(void) {
    return cache_mgmt_info.cache_allocation_supported;
}

u8 cache_mgmt_is_cache_monitoring_supported(void) {
    return cache_mgmt_info.cache_monitoring_supported;
}

u32 cache_mgmt_get_total_cache_size(void) {
    u32 total_size = 0;
    
    for (u8 i = 0; i < cache_mgmt_info.num_cache_levels; i++) {
        total_size += cache_mgmt_info.caches[i].size;
    }
    
    return total_size;
}

u8 cache_mgmt_is_cache_shared(u8 level) {
    const cache_descriptor_t* cache = cache_mgmt_get_cache_info(level, 0);
    return cache ? (cache->shared_cores > 1) : 0;
}

u8 cache_mgmt_get_shared_cores(u8 level) {
    const cache_descriptor_t* cache = cache_mgmt_get_cache_info(level, 0);
    return cache ? cache->shared_cores : 0;
}

void cache_mgmt_disable_cache(void) {
    u32 cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= (1 << 30); /* Set CD bit */
    cr0 |= (1 << 29); /* Set NW bit */
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
    __asm__ volatile("wbinvd" : : : "memory");
}

void cache_mgmt_enable_cache(void) {
    u32 cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1 << 30); /* Clear CD bit */
    cr0 &= ~(1 << 29); /* Clear NW bit */
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
}

u8 cache_mgmt_is_cache_enabled(void) {
    u32 cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    return !(cr0 & (1 << 30)); /* Check CD bit */
}
