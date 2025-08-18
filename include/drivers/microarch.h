/*
 * microarch.h – x86 CPU microarchitecture control (branch prediction/speculation)
 */

#ifndef MICROARCH_H
#define MICROARCH_H

#include "kernel/types.h"

typedef struct {
    u64 total_branches;
    u64 branch_misses;
    u64 indirect_branches;
    u64 indirect_misses;
    u32 misprediction_rate;
    u64 last_measurement;
} branch_stats_t;

/* Core functions */
void microarch_init(void);

/* Support detection */
u8 microarch_is_supported(void);
u8 microarch_is_spec_ctrl_supported(void);
u8 microarch_is_ibrs_supported(void);
u8 microarch_is_stibp_supported(void);
u8 microarch_is_ssbd_supported(void);

/* Speculation control */
void microarch_enable_ibrs(void);
void microarch_disable_ibrs(void);
void microarch_enable_stibp(void);
void microarch_disable_stibp(void);
void microarch_enable_ssbd(void);
void microarch_disable_ssbd(void);

/* Branch prediction control */
void microarch_flush_indirect_branches(void);

/* Status */
u8 microarch_is_ibrs_enabled(void);
u8 microarch_is_stibp_enabled(void);
u8 microarch_is_ssbd_enabled(void);

/* Information */
u64 microarch_get_arch_capabilities(void);
u64 microarch_get_spec_ctrl_value(void);

/* Branch statistics */
void microarch_update_branch_stats(void);
const branch_stats_t* microarch_get_branch_stats(void);

/* LBR support */
void microarch_read_lbr_stack(void);
const u64* microarch_get_lbr_entries(void);
u8 microarch_get_lbr_depth(void);

/* Statistics */
u32 microarch_get_speculation_mitigations(void);
u32 microarch_get_branch_flushes(void);
u64 microarch_get_last_ibpb_time(void);

/* Vulnerability checking */
u8 microarch_check_vulnerability(u32 vulnerability);

/* Convenience functions */
void microarch_enable_all_mitigations(void);
void microarch_disable_all_mitigations(void);
u8 microarch_is_branch_prediction_enabled(void);
void microarch_clear_statistics(void);

#endif
