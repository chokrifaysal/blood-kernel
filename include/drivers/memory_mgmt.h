/*
 * memory_mgmt.h – x86 advanced memory management (PAT/MTRR extensions)
 */

#ifndef MEMORY_MGMT_H
#define MEMORY_MGMT_H

#include "kernel/types.h"

/* Memory type definitions */
#define MEMORY_TYPE_UC          0   /* Uncacheable */
#define MEMORY_TYPE_WC          1   /* Write Combining */
#define MEMORY_TYPE_WT          4   /* Write Through */
#define MEMORY_TYPE_WP          5   /* Write Protected */
#define MEMORY_TYPE_WB          6   /* Write Back */
#define MEMORY_TYPE_UC_MINUS    7   /* Uncacheable Minus */

typedef struct {
    u64 base;
    u64 size;
    u8 type;
    u8 valid;
} memory_range_t;

/* Core functions */
void memory_mgmt_init(void);

/* Support detection */
u8 memory_mgmt_is_supported(void);
u8 memory_mgmt_is_pat_supported(void);
u8 memory_mgmt_is_mtrr_supported(void);
u8 memory_mgmt_is_wc_supported(void);
u8 memory_mgmt_is_fixed_mtrrs_supported(void);

/* MTRR management */
u8 memory_mgmt_get_num_variable_mtrrs(void);
u8 memory_mgmt_set_variable_mtrr(u8 index, u64 base, u64 size, u8 type);
u8 memory_mgmt_clear_variable_mtrr(u8 index);
const memory_range_t* memory_mgmt_get_variable_range(u8 index);
const memory_range_t* memory_mgmt_get_fixed_range(u8 index);

/* PAT management */
u64 memory_mgmt_get_pat_value(void);
u8 memory_mgmt_get_default_memory_type(void);
u8 memory_mgmt_get_memory_type(u64 address);
u32 memory_mgmt_get_page_attributes(u8 memory_type);

/* TLB management */
void memory_mgmt_flush_tlb(void);
void memory_mgmt_flush_tlb_single(u64 address);

/* Advanced features */
u8 memory_mgmt_is_memory_encryption_supported(void);
u8 memory_mgmt_is_protection_keys_supported(void);
u8 memory_mgmt_is_smrr_supported(void);

/* MTRR control */
void memory_mgmt_enable_mtrrs(void);
void memory_mgmt_disable_mtrrs(void);

#endif
