/*
 * cache_mgmt.h – x86 CPU cache management (L1/L2/L3 control)
 */

#ifndef CACHE_MGMT_H
#define CACHE_MGMT_H

#include "kernel/types.h"

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

/* Core functions */
void cache_mgmt_init(void);

/* Support detection */
u8 cache_mgmt_is_supported(void);
u8 cache_mgmt_get_num_levels(void);

/* Cache information */
const cache_descriptor_t* cache_mgmt_get_cache_info(u8 level, u8 type);
u32 cache_mgmt_get_cache_size(u8 level, u8 type);
u16 cache_mgmt_get_line_size(u8 level, u8 type);
u16 cache_mgmt_get_associativity(u8 level, u8 type);
u32 cache_mgmt_get_total_cache_size(void);

/* Cache control */
void cache_mgmt_flush_all(void);
void cache_mgmt_flush_line(void* address);
void cache_mgmt_writeback_line(void* address);
void cache_mgmt_flush_range(void* start, u32 size);
void cache_mgmt_writeback_range(void* start, u32 size);

/* Cache prefetching */
void cache_mgmt_prefetch(void* address, u8 locality);
void cache_mgmt_prefetch_range(void* start, u32 size, u8 locality);

/* Feature support */
u8 cache_mgmt_is_clflush_supported(void);
u8 cache_mgmt_is_clflushopt_supported(void);
u8 cache_mgmt_is_clwb_supported(void);
u8 cache_mgmt_is_cache_allocation_supported(void);
u8 cache_mgmt_is_cache_monitoring_supported(void);

/* Cache sharing */
u8 cache_mgmt_is_cache_shared(u8 level);
u8 cache_mgmt_get_shared_cores(u8 level);

/* Cache enable/disable */
void cache_mgmt_disable_cache(void);
void cache_mgmt_enable_cache(void);
u8 cache_mgmt_is_cache_enabled(void);

#endif
