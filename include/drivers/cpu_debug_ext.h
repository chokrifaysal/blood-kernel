/*
 * cpu_debug_ext.h – x86 CPU debugging extensions (Intel PT/LBR/BTS)
 */

#ifndef CPU_DEBUG_EXT_H
#define CPU_DEBUG_EXT_H

#include "kernel/types.h"

/* LBR Select bits */
#define LBR_SELECT_CPL_EQ_0             (1ULL << 0)
#define LBR_SELECT_CPL_NEQ_0            (1ULL << 1)
#define LBR_SELECT_JCC                  (1ULL << 2)
#define LBR_SELECT_NEAR_REL_CALL        (1ULL << 3)
#define LBR_SELECT_NEAR_IND_CALL        (1ULL << 4)
#define LBR_SELECT_NEAR_RET             (1ULL << 5)
#define LBR_SELECT_NEAR_IND_JMP         (1ULL << 6)
#define LBR_SELECT_NEAR_REL_JMP         (1ULL << 7)
#define LBR_SELECT_FAR_BRANCH           (1ULL << 8)
#define LBR_SELECT_EN_CALLSTACK         (1ULL << 9)

typedef struct {
    u64 from_ip;
    u64 to_ip;
    u64 info;
    u64 timestamp;
} lbr_entry_t;

typedef struct {
    u64 from_ip;
    u64 to_ip;
    u64 flags;
} bts_entry_t;

/* Core functions */
void cpu_debug_ext_init(void);

/* Support detection */
u8 cpu_debug_ext_is_supported(void);
u8 cpu_debug_ext_is_intel_pt_supported(void);
u8 cpu_debug_ext_is_lbr_supported(void);
u8 cpu_debug_ext_is_bts_supported(void);

/* Intel Processor Trace (PT) */
u8 cpu_debug_ext_enable_intel_pt(u8 user_mode, u8 kernel_mode);
u8 cpu_debug_ext_disable_intel_pt(void);
u8 cpu_debug_ext_is_intel_pt_enabled(void);
u64 cpu_debug_ext_get_pt_output_base(void);
u64 cpu_debug_ext_get_pt_status(void);

/* Last Branch Record (LBR) */
u8 cpu_debug_ext_enable_lbr(u64 select_mask);
u8 cpu_debug_ext_disable_lbr(void);
u8 cpu_debug_ext_is_lbr_enabled(void);
u8 cpu_debug_ext_get_lbr_depth(void);
void cpu_debug_ext_read_lbr_stack(void);
const lbr_entry_t* cpu_debug_ext_get_lbr_entry(u8 index);
u32 cpu_debug_ext_get_lbr_tos(void);

/* Branch Trace Store (BTS) */
u8 cpu_debug_ext_enable_bts(void* buffer, u32 buffer_size);
u8 cpu_debug_ext_disable_bts(void);
u8 cpu_debug_ext_is_bts_enabled(void);

/* Interrupt handling */
void cpu_debug_ext_handle_debug_interrupt(void);

/* Statistics */
u32 cpu_debug_ext_get_total_trace_sessions(void);
u64 cpu_debug_ext_get_total_trace_time(void);
u32 cpu_debug_ext_get_debug_interrupts(void);
u32 cpu_debug_ext_get_lbr_records_captured(void);
u32 cpu_debug_ext_get_bts_records_captured(void);
u32 cpu_debug_ext_get_bts_buffer_overflows(void);

/* Utilities */
void cpu_debug_ext_clear_statistics(void);

#endif
