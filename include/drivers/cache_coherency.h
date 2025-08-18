/*
 * cache_coherency.h – x86 CPU cache coherency protocols (MESI/MOESI)
 */

#ifndef CACHE_COHERENCY_H
#define CACHE_COHERENCY_H

#include "kernel/types.h"

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

/* Core functions */
void cache_coherency_init(void);

/* Support detection */
u8 cache_coherency_is_supported(void);
u8 cache_coherency_is_mesi_supported(void);
u8 cache_coherency_is_moesi_supported(void);
u8 cache_coherency_is_monitoring_supported(void);
u8 cache_coherency_is_prefetch_supported(void);

/* Cache information */
u8 cache_coherency_get_num_levels(void);
const cache_level_info_t* cache_coherency_get_level_info(u8 level);

/* Cache operations */
void cache_coherency_flush_line(u64 address);
void cache_coherency_flush_range(u64 start, u64 size);
void cache_coherency_invalidate_line(u64 address);
void cache_coherency_writeback_line(u64 address);
void cache_coherency_prefetch_line(u64 address, u8 hint);

/* Cache monitoring */
u8 cache_coherency_monitor_line(u64 address);
void cache_coherency_update_line_state(u64 address, u8 new_state);
const cache_line_info_t* cache_coherency_get_monitored_line(u8 index);
u8 cache_coherency_get_monitored_line_count(void);
void cache_coherency_clear_monitoring(void);

/* Coherency protocol */
void cache_coherency_handle_snoop_hit(u64 address);
void cache_coherency_handle_snoop_miss(u64 address);

/* Statistics */
u32 cache_coherency_get_total_operations(void);
u32 cache_coherency_get_coherency_transactions(void);
u32 cache_coherency_get_snoop_hits(void);
u32 cache_coherency_get_snoop_misses(void);
u32 cache_coherency_get_cache_flushes(void);
u32 cache_coherency_get_cache_invalidations(void);
u64 cache_coherency_get_last_event_time(void);

/* Cache control */
void cache_coherency_disable_cache(void);
void cache_coherency_enable_cache(void);

/* Utilities */
void cache_coherency_clear_statistics(void);
const char* cache_coherency_get_state_name(u8 state);

#endif
