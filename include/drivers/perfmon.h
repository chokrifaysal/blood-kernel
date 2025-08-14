/*
 * perfmon.h – x86 performance monitoring and profiling
 */

#ifndef PERFMON_H
#define PERFMON_H

#include "kernel/types.h"

/* Common performance events */
#define PERF_EVENT_CYCLES           0x3C  /* CPU cycles */
#define PERF_EVENT_INSTRUCTIONS     0xC0  /* Instructions retired */
#define PERF_EVENT_CACHE_REFERENCES 0x2E  /* Cache references */
#define PERF_EVENT_CACHE_MISSES     0x2E  /* Cache misses */
#define PERF_EVENT_BRANCH_INSTRUCTIONS 0xC4  /* Branch instructions */
#define PERF_EVENT_BRANCH_MISSES    0xC5  /* Branch mispredictions */

/* UMASK values for cache events */
#define UMASK_CACHE_LL_REFS         0x4F  /* Last level cache references */
#define UMASK_CACHE_LL_MISSES       0x41  /* Last level cache misses */
#define UMASK_CACHE_L1D_REFS        0x01  /* L1 data cache references */
#define UMASK_CACHE_L1D_MISSES      0x08  /* L1 data cache misses */

/* Core functions */
void perfmon_init(void);
u8 perfmon_is_supported(void);

/* Counter setup */
void perfmon_setup_counter(u8 counter, u32 event, u32 umask, u8 user_mode, u8 kernel_mode, const char* name);

/* Programmable counter control */
void perfmon_enable_counter(u8 counter);
void perfmon_disable_counter(u8 counter);
u64 perfmon_read_counter(u8 counter);
void perfmon_reset_counter(u8 counter);

/* Fixed counter control */
void perfmon_enable_fixed_counter(u8 counter);
void perfmon_disable_fixed_counter(u8 counter);
u64 perfmon_read_fixed_counter(u8 counter);
void perfmon_reset_fixed_counter(u8 counter);

/* Global control */
void perfmon_enable_all(void);
void perfmon_disable_all(void);

/* Information functions */
u8 perfmon_get_version(void);
u8 perfmon_get_num_counters(void);
u8 perfmon_get_counter_width(void);
u8 perfmon_get_num_fixed_counters(void);
const char* perfmon_get_counter_name(u8 counter);
const char* perfmon_get_fixed_counter_name(u8 counter);

/* Status functions */
u8 perfmon_is_counter_enabled(u8 counter);
u8 perfmon_is_fixed_counter_enabled(u8 counter);

#endif
