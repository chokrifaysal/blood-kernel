/*
 * debug.h – x86 advanced debugging features (LBR, BTS, PMU)
 */

#ifndef DEBUG_H
#define DEBUG_H

#include "kernel/types.h"

/* LBR filter bits */
#define LBR_SELECT_CPL_EQ_0     0x01    /* CPL = 0 */
#define LBR_SELECT_CPL_NEQ_0    0x02    /* CPL != 0 */
#define LBR_SELECT_JCC          0x04    /* Conditional branches */
#define LBR_SELECT_NEAR_REL_CALL 0x08   /* Near relative calls */
#define LBR_SELECT_NEAR_IND_CALL 0x10   /* Near indirect calls */
#define LBR_SELECT_NEAR_RET     0x20    /* Near returns */
#define LBR_SELECT_NEAR_IND_JMP 0x40    /* Near indirect jumps */
#define LBR_SELECT_NEAR_REL_JMP 0x80    /* Near relative jumps */
#define LBR_SELECT_FAR_BRANCH   0x100   /* Far branches */

typedef struct {
    u64 from_ip;
    u64 to_ip;
    u64 info;
} lbr_entry_t;

typedef struct {
    u64 from_ip;
    u64 to_ip;
    u64 flags;
} bts_entry_t;

/* Core functions */
void debug_init(void);

/* Support detection */
u8 debug_is_lbr_supported(void);
u8 debug_is_bts_supported(void);

/* Status functions */
u8 debug_is_lbr_enabled(void);
u8 debug_is_bts_enabled(void);

/* LBR (Last Branch Record) control */
void debug_enable_lbr(void);
void debug_disable_lbr(void);
void debug_set_lbr_filter(u32 filter_mask);
u32 debug_get_lbr_filter(void);
u8 debug_get_lbr_depth(void);
u8 debug_get_lbr_format(void);

/* LBR data access */
u32 debug_read_lbr_stack(lbr_entry_t* buffer, u32 max_entries);
void debug_clear_lbr_stack(void);
u64 debug_get_last_branch_from(void);
u64 debug_get_last_branch_to(void);

/* BTS (Branch Trace Store) control */
void debug_enable_bts(void);
void debug_disable_bts(void);
u32 debug_get_bts_buffer_size(void);

/* BTS data access */
u32 debug_read_bts_buffer(bts_entry_t* buffer, u32 max_entries);
void debug_clear_bts_buffer(void);

/* Advanced debugging features */
void debug_freeze_on_pmi(u8 enable);
void debug_single_step_branches(u8 enable);
u64 debug_get_debugctl(void);

/* Interrupt handler */
void debug_bts_interrupt_handler(void);

#endif
