/*
 * cache_coherency.c – x86 CPU cache coherency protocols (MESI/MOESI)
 */

#include "kernel/types.h"

/* Cache coherency MSRs */
#define MSR_CACHE_DISABLE               0x2FF
#define MSR_MTRR_CAP                    0xFE
#define MSR_MTRR_DEF_TYPE               0x2FF
#define MSR_MTRR_PHYSBASE0              0x200
#define MSR_MTRR_PHYSMASK0              0x201

/* Cache control bits */
#define CR0_CD                          (1 << 30)
#define CR0_NW                          (1 << 29)

/* MTRR type definitions */
#define MTRR_TYPE_UC                    0x00
#define MTRR_TYPE_WC                    0x01
#define MTRR_TYPE_WT                    0x04
#define MTRR_TYPE_WP                    0x05
#define MTRR_TYPE_WB                    0x06

/* Cache line states (MESI/MOESI) */
#define CACHE_STATE_INVALID             0
#define CACHE_STATE_SHARED              1
#define CACHE_STATE_EXCLUSIVE           2
#define CACHE_STATE_MODIFIED            3
#define CACHE_STATE_OWNED               4  /* MOESI only */

/* Cache operations */
#define CACHE_OP_FLUSH                  0
#define CACHE_OP_INVALIDATE             1
#define CACHE_OP_WRITEBACK              2
#define CACHE_OP_PREFETCH               3

typedef struct {
    u64 address;
    u8 state;
    u8 level;
    u32 access_count;
    u64 last_access;
    u8 dirty;
} cache_line_info_t;

typedef struct {
    u32 size_kb;
    u32 line_size;
    u32 associativity;
    u32 sets;
    u8 level;
    u8 type;  /* 0=data, 1=instruction, 2=unified */
    u8 inclusive;
    u32 hit_count;
    u32 miss_count;
    u32 eviction_count;
    u32 coherency_misses;
} cache_level_info_t;

typedef struct {
    u8 cache_coherency_supported;
    u8 mesi_supported;
    u8 moesi_supported;
    u8 cache_monitoring_supported;
    u8 prefetch_supported;
    u8 num_cache_levels;
    cache_level_info_t levels[4];
    u32 total_cache_operations;
    u32 coherency_transactions;
    u32 snoop_hits;
    u32 snoop_misses;
    u32 cache_flushes;
    u32 cache_invalidations;
    u64 last_coherency_event;
    cache_line_info_t monitored_lines[64];
    u8 monitored_line_count;
} cache_coherency_info_t;

static cache_coherency_info_t cache_coherency_info;

extern u64 msr_read(u32 msr);
extern void msr_write(u32 msr, u64 value);
extern u8 msr_is_supported(void);
extern u64 timer_get_ticks(void);

static void cache_coherency_detect_capabilities(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check for cache monitoring support */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    cache_coherency_info.cache_monitoring_supported = (ebx & (1 << 12)) != 0;
    
    /* Check for prefetch support */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    cache_coherency_info.prefetch_supported = (ecx & (1 << 19)) != 0;
    
    /* Detect cache hierarchy */
    u8 level = 0;
    for (u8 i = 0; i < 4; i++) {
        __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(4), "c"(i));
        
        u8 cache_type = eax & 0x1F;
        if (cache_type == 0) break;  /* No more cache levels */
        
        cache_coherency_info.levels[level].level = ((eax >> 5) & 0x7) + 1;
        cache_coherency_info.levels[level].type = cache_type - 1;
        cache_coherency_info.levels[level].line_size = (ebx & 0xFFF) + 1;
        cache_coherency_info.levels[level].associativity = ((ebx >> 22) & 0x3FF) + 1;
        cache_coherency_info.levels[level].sets = ecx + 1;
        cache_coherency_info.levels[level].inclusive = (edx & (1 << 1)) != 0;
        
        u32 size = cache_coherency_info.levels[level].line_size *
                   cache_coherency_info.levels[level].associativity *
                   cache_coherency_info.levels[level].sets;
        cache_coherency_info.levels[level].size_kb = size / 1024;
        
        level++;
    }
    
    cache_coherency_info.num_cache_levels = level;
    cache_coherency_info.cache_coherency_supported = (level > 0);
    
    /* Assume MESI for Intel, MOESI for AMD */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0));
    if (ebx == 0x756E6547) {  /* "Genu" - Intel */
        cache_coherency_info.mesi_supported = 1;
    } else if (ebx == 0x68747541) {  /* "Auth" - AMD */
        cache_coherency_info.moesi_supported = 1;
    }
}

void cache_coherency_init(void) {
    cache_coherency_info.cache_coherency_supported = 0;
    cache_coherency_info.mesi_supported = 0;
    cache_coherency_info.moesi_supported = 0;
    cache_coherency_info.cache_monitoring_supported = 0;
    cache_coherency_info.prefetch_supported = 0;
    cache_coherency_info.num_cache_levels = 0;
    cache_coherency_info.total_cache_operations = 0;
    cache_coherency_info.coherency_transactions = 0;
    cache_coherency_info.snoop_hits = 0;
    cache_coherency_info.snoop_misses = 0;
    cache_coherency_info.cache_flushes = 0;
    cache_coherency_info.cache_invalidations = 0;
    cache_coherency_info.last_coherency_event = 0;
    cache_coherency_info.monitored_line_count = 0;
    
    for (u8 i = 0; i < 4; i++) {
        cache_coherency_info.levels[i].size_kb = 0;
        cache_coherency_info.levels[i].line_size = 0;
        cache_coherency_info.levels[i].associativity = 0;
        cache_coherency_info.levels[i].sets = 0;
        cache_coherency_info.levels[i].level = 0;
        cache_coherency_info.levels[i].type = 0;
        cache_coherency_info.levels[i].inclusive = 0;
        cache_coherency_info.levels[i].hit_count = 0;
        cache_coherency_info.levels[i].miss_count = 0;
        cache_coherency_info.levels[i].eviction_count = 0;
        cache_coherency_info.levels[i].coherency_misses = 0;
    }
    
    for (u8 i = 0; i < 64; i++) {
        cache_coherency_info.monitored_lines[i].address = 0;
        cache_coherency_info.monitored_lines[i].state = CACHE_STATE_INVALID;
        cache_coherency_info.monitored_lines[i].level = 0;
        cache_coherency_info.monitored_lines[i].access_count = 0;
        cache_coherency_info.monitored_lines[i].last_access = 0;
        cache_coherency_info.monitored_lines[i].dirty = 0;
    }
    
    cache_coherency_detect_capabilities();
}

u8 cache_coherency_is_supported(void) {
    return cache_coherency_info.cache_coherency_supported;
}

u8 cache_coherency_is_mesi_supported(void) {
    return cache_coherency_info.mesi_supported;
}

u8 cache_coherency_is_moesi_supported(void) {
    return cache_coherency_info.moesi_supported;
}

u8 cache_coherency_get_num_levels(void) {
    return cache_coherency_info.num_cache_levels;
}

const cache_level_info_t* cache_coherency_get_level_info(u8 level) {
    if (level >= cache_coherency_info.num_cache_levels) return 0;
    return &cache_coherency_info.levels[level];
}

void cache_coherency_flush_line(u64 address) {
    __asm__ volatile("clflush (%0)" : : "r"(address) : "memory");
    cache_coherency_info.cache_flushes++;
    cache_coherency_info.total_cache_operations++;
    cache_coherency_info.last_coherency_event = timer_get_ticks();
}

void cache_coherency_flush_range(u64 start, u64 size) {
    u32 line_size = 64;  /* Assume 64-byte cache lines */
    if (cache_coherency_info.num_cache_levels > 0) {
        line_size = cache_coherency_info.levels[0].line_size;
    }
    
    u64 end = start + size;
    for (u64 addr = start & ~(line_size - 1); addr < end; addr += line_size) {
        cache_coherency_flush_line(addr);
    }
}

void cache_coherency_invalidate_line(u64 address) {
    /* Use INVD instruction for invalidation */
    __asm__ volatile("invd" : : : "memory");
    cache_coherency_info.cache_invalidations++;
    cache_coherency_info.total_cache_operations++;
    cache_coherency_info.last_coherency_event = timer_get_ticks();
}

void cache_coherency_writeback_line(u64 address) {
    __asm__ volatile("clwb (%0)" : : "r"(address) : "memory");
    cache_coherency_info.total_cache_operations++;
    cache_coherency_info.last_coherency_event = timer_get_ticks();
}

void cache_coherency_prefetch_line(u64 address, u8 hint) {
    if (!cache_coherency_info.prefetch_supported) return;
    
    switch (hint) {
        case 0:  /* Temporal locality */
            __asm__ volatile("prefetcht0 (%0)" : : "r"(address));
            break;
        case 1:  /* Low temporal locality */
            __asm__ volatile("prefetcht1 (%0)" : : "r"(address));
            break;
        case 2:  /* Minimal temporal locality */
            __asm__ volatile("prefetcht2 (%0)" : : "r"(address));
            break;
        case 3:  /* Non-temporal */
            __asm__ volatile("prefetchnta (%0)" : : "r"(address));
            break;
    }
    
    cache_coherency_info.total_cache_operations++;
}

u8 cache_coherency_monitor_line(u64 address) {
    if (cache_coherency_info.monitored_line_count >= 64) return 0;
    
    cache_line_info_t* line = &cache_coherency_info.monitored_lines[cache_coherency_info.monitored_line_count];
    line->address = address;
    line->state = CACHE_STATE_EXCLUSIVE;  /* Assume exclusive on first access */
    line->level = 1;
    line->access_count = 1;
    line->last_access = timer_get_ticks();
    line->dirty = 0;
    
    cache_coherency_info.monitored_line_count++;
    return 1;
}

void cache_coherency_update_line_state(u64 address, u8 new_state) {
    for (u8 i = 0; i < cache_coherency_info.monitored_line_count; i++) {
        if (cache_coherency_info.monitored_lines[i].address == address) {
            cache_coherency_info.monitored_lines[i].state = new_state;
            cache_coherency_info.monitored_lines[i].access_count++;
            cache_coherency_info.monitored_lines[i].last_access = timer_get_ticks();
            
            if (new_state == CACHE_STATE_MODIFIED) {
                cache_coherency_info.monitored_lines[i].dirty = 1;
            }
            
            cache_coherency_info.coherency_transactions++;
            break;
        }
    }
}

void cache_coherency_handle_snoop_hit(u64 address) {
    cache_coherency_info.snoop_hits++;
    cache_coherency_info.coherency_transactions++;
    
    /* Update line state based on snoop result */
    for (u8 i = 0; i < cache_coherency_info.monitored_line_count; i++) {
        if (cache_coherency_info.monitored_lines[i].address == address) {
            if (cache_coherency_info.monitored_lines[i].state == CACHE_STATE_EXCLUSIVE) {
                cache_coherency_info.monitored_lines[i].state = CACHE_STATE_SHARED;
            }
            break;
        }
    }
}

void cache_coherency_handle_snoop_miss(u64 address) {
    cache_coherency_info.snoop_misses++;
    cache_coherency_info.coherency_transactions++;
}

u32 cache_coherency_get_total_operations(void) {
    return cache_coherency_info.total_cache_operations;
}

u32 cache_coherency_get_coherency_transactions(void) {
    return cache_coherency_info.coherency_transactions;
}

u32 cache_coherency_get_snoop_hits(void) {
    return cache_coherency_info.snoop_hits;
}

u32 cache_coherency_get_snoop_misses(void) {
    return cache_coherency_info.snoop_misses;
}

u32 cache_coherency_get_cache_flushes(void) {
    return cache_coherency_info.cache_flushes;
}

u32 cache_coherency_get_cache_invalidations(void) {
    return cache_coherency_info.cache_invalidations;
}

u64 cache_coherency_get_last_event_time(void) {
    return cache_coherency_info.last_coherency_event;
}

const cache_line_info_t* cache_coherency_get_monitored_line(u8 index) {
    if (index >= cache_coherency_info.monitored_line_count) return 0;
    return &cache_coherency_info.monitored_lines[index];
}

u8 cache_coherency_get_monitored_line_count(void) {
    return cache_coherency_info.monitored_line_count;
}

void cache_coherency_clear_monitoring(void) {
    cache_coherency_info.monitored_line_count = 0;
    for (u8 i = 0; i < 64; i++) {
        cache_coherency_info.monitored_lines[i].address = 0;
        cache_coherency_info.monitored_lines[i].state = CACHE_STATE_INVALID;
        cache_coherency_info.monitored_lines[i].access_count = 0;
    }
}

void cache_coherency_clear_statistics(void) {
    cache_coherency_info.total_cache_operations = 0;
    cache_coherency_info.coherency_transactions = 0;
    cache_coherency_info.snoop_hits = 0;
    cache_coherency_info.snoop_misses = 0;
    cache_coherency_info.cache_flushes = 0;
    cache_coherency_info.cache_invalidations = 0;
    
    for (u8 i = 0; i < cache_coherency_info.num_cache_levels; i++) {
        cache_coherency_info.levels[i].hit_count = 0;
        cache_coherency_info.levels[i].miss_count = 0;
        cache_coherency_info.levels[i].eviction_count = 0;
        cache_coherency_info.levels[i].coherency_misses = 0;
    }
}

const char* cache_coherency_get_state_name(u8 state) {
    switch (state) {
        case CACHE_STATE_INVALID: return "Invalid";
        case CACHE_STATE_SHARED: return "Shared";
        case CACHE_STATE_EXCLUSIVE: return "Exclusive";
        case CACHE_STATE_MODIFIED: return "Modified";
        case CACHE_STATE_OWNED: return "Owned";
        default: return "Unknown";
    }
}

u8 cache_coherency_is_monitoring_supported(void) {
    return cache_coherency_info.cache_monitoring_supported;
}

u8 cache_coherency_is_prefetch_supported(void) {
    return cache_coherency_info.prefetch_supported;
}

void cache_coherency_disable_cache(void) {
    u64 cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= CR0_CD | CR0_NW;
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
    __asm__ volatile("wbinvd");
}

void cache_coherency_enable_cache(void) {
    u64 cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(CR0_CD | CR0_NW);
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
}
