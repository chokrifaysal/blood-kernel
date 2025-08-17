/*
 * memory_adv.h – x86 advanced memory management (PCID/INVPCID)
 */

#ifndef MEMORY_ADV_H
#define MEMORY_ADV_H

#include "kernel/types.h"

/* INVPCID types */
#define INVPCID_TYPE_INDIVIDUAL     0
#define INVPCID_TYPE_SINGLE_CONTEXT 1
#define INVPCID_TYPE_ALL_GLOBAL     2
#define INVPCID_TYPE_ALL_NON_GLOBAL 3

/* PCID constants */
#define PCID_KERNEL                 0
#define PCID_USER_BASE              1

/* Memory types */
#define MEMORY_TYPE_UC              0x00
#define MEMORY_TYPE_WC              0x01
#define MEMORY_TYPE_WT              0x04
#define MEMORY_TYPE_WP              0x05
#define MEMORY_TYPE_WB              0x06

typedef struct {
    u16 pcid;
    u64 cr3_base;
    u8 active;
    u8 global;
    u32 tlb_flushes;
    u64 last_used;
} pcid_entry_t;

typedef struct {
    u64 base_address;
    u64 size;
    u8 memory_type;
    u8 enabled;
} mtrr_entry_t;

/* Core functions */
void memory_adv_init(void);

/* Support detection */
u8 memory_adv_is_supported(void);
u8 memory_adv_is_pcid_supported(void);
u8 memory_adv_is_invpcid_supported(void);
u8 memory_adv_is_pcid_enabled(void);

/* PCID management */
u16 memory_adv_allocate_pcid(u64 cr3_base);
void memory_adv_free_pcid(u16 pcid);
void memory_adv_switch_pcid(u16 pcid, u64 cr3_base);
u16 memory_adv_get_current_pcid(void);

/* TLB management */
void memory_adv_invpcid(u32 type, u16 pcid, u64 linear_address);
void memory_adv_flush_tlb_pcid(u16 pcid);
void memory_adv_flush_tlb_all(void);

/* PAT support */
u8 memory_adv_is_pat_supported(void);
u64 memory_adv_get_pat_value(void);
void memory_adv_set_pat_entry(u8 index, u8 memory_type);
u8 memory_adv_get_pat_entry(u8 index);

/* MTRR support */
u8 memory_adv_is_mtrr_supported(void);
u8 memory_adv_setup_mtrr(u8 index, u64 base_address, u64 size, u8 memory_type);
void memory_adv_disable_mtrr(u8 index);
const mtrr_entry_t* memory_adv_get_mtrr_entry(u8 index);
u8 memory_adv_get_num_mtrr_entries(void);

/* PCID information */
const pcid_entry_t* memory_adv_get_pcid_entry(u8 index);

/* Statistics */
u32 memory_adv_get_total_tlb_flushes(void);
u32 memory_adv_get_pcid_switches(void);
u32 memory_adv_get_invpcid_calls(void);
void memory_adv_clear_statistics(void);

/* Utilities */
u8 memory_adv_find_free_pcid_slot(void);
u8 memory_adv_find_free_mtrr_slot(void);
void memory_adv_set_pcid_global(u16 pcid, u8 global);
u8 memory_adv_is_pcid_global(u16 pcid);

#endif
