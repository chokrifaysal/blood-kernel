/*
 * cache.h – x86 CPU cache management and control
 */

#ifndef CACHE_H
#define CACHE_H

#include "kernel/types.h"

/* MTRR memory types */
#define MTRR_TYPE_UC            0x00  /* Uncacheable */
#define MTRR_TYPE_WC            0x01  /* Write Combining */
#define MTRR_TYPE_WT            0x04  /* Write Through */
#define MTRR_TYPE_WP            0x05  /* Write Protected */
#define MTRR_TYPE_WB            0x06  /* Write Back */

/* Core functions */
void cache_init(void);

/* Cache control */
void cache_enable(void);
void cache_disable(void);
void cache_flush_all(void);
void cache_invalidate_all(void);
void cache_flush_line(void* address);

/* MTRR management */
u8 cache_setup_mtrr(u8 reg, u64 base, u64 size, u8 type);
void cache_clear_mtrr(u8 reg);
void cache_enable_mtrrs(void);
void cache_disable_mtrrs(void);

/* Cache information */
u8 cache_is_mtrr_supported(void);
u8 cache_is_pat_supported(void);
u8 cache_get_num_variable_mtrrs(void);
u32 cache_get_line_size(void);
u32 cache_get_l1_size(void);
u32 cache_get_l2_size(void);
u32 cache_get_l3_size(void);

/* MTRR status */
u8 cache_get_mtrr_type(u8 reg);
u64 cache_get_mtrr_base(u8 reg);
u64 cache_get_mtrr_mask(u8 reg);
u8 cache_is_mtrr_valid(u8 reg);

/* Prefetch operations */
void cache_prefetch(void* address);
void cache_prefetch_nta(void* address);

#endif
