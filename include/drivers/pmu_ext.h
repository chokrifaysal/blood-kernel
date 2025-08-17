/*
 * pmu_ext.h – x86 advanced performance monitoring unit extensions
 */

#ifndef PMU_EXT_H
#define PMU_EXT_H

#include "kernel/types.h"

/* Common performance events */
#define PMU_EVENT_CYCLES            0x3C
#define PMU_EVENT_INSTRUCTIONS      0xC0
#define PMU_EVENT_CACHE_REFERENCES  0x2E
#define PMU_EVENT_CACHE_MISSES      0x2E
#define PMU_EVENT_BRANCH_INSTRUCTIONS 0xC4
#define PMU_EVENT_BRANCH_MISSES     0xC5
#define PMU_EVENT_BUS_CYCLES        0x3C
#define PMU_EVENT_STALLED_CYCLES_FRONTEND 0x9C
#define PMU_EVENT_STALLED_CYCLES_BACKEND  0xA1

typedef struct {
    u32 event;
    u32 umask;
    u8 user_mode;
    u8 kernel_mode;
    u8 edge_detect;
    u8 pin_control;
    u8 interrupt_enable;
    u8 any_thread;
    u8 invert;
    u8 counter_mask;
    u64 count;
    u64 sample_period;
    u32 overflow_count;
} pmu_counter_t;

typedef struct {
    u64 from_ip;
    u64 to_ip;
    u64 info;
} lbr_entry_t;

/* Core functions */
void pmu_ext_init(void);

/* Support detection */
u8 pmu_ext_is_supported(void);
u8 pmu_ext_get_version(void);
u8 pmu_ext_get_num_gp_counters(void);
u8 pmu_ext_get_num_fixed_counters(void);
u8 pmu_ext_is_pebs_supported(void);
u8 pmu_ext_is_lbr_supported(void);

/* General-purpose counters */
u8 pmu_ext_setup_gp_counter(u8 index, u32 event, u32 umask, u8 user, u8 kernel);
u8 pmu_ext_enable_counter(u8 index);
u8 pmu_ext_disable_counter(u8 index);
u64 pmu_ext_read_counter(u8 index);
void pmu_ext_reset_counter(u8 index);
const pmu_counter_t* pmu_ext_get_gp_counter_info(u8 index);

/* Fixed-function counters */
u8 pmu_ext_setup_fixed_counter(u8 index, u8 user, u8 kernel, u8 enable_pmi);
u64 pmu_ext_read_fixed_counter(u8 index);
const pmu_counter_t* pmu_ext_get_fixed_counter_info(u8 index);

/* Last Branch Record (LBR) */
void pmu_ext_enable_lbr(void);
void pmu_ext_disable_lbr(void);
void pmu_ext_read_lbr_stack(void);
const lbr_entry_t* pmu_ext_get_lbr_entry(u8 index);
u8 pmu_ext_get_lbr_tos(void);
u8 pmu_ext_get_lbr_depth(void);

/* Performance Monitoring Interrupt (PMI) */
void pmu_ext_handle_pmi(void);
u32 pmu_ext_get_pmi_count(void);
u32 pmu_ext_get_overflow_count(void);

/* Global control */
u64 pmu_ext_get_global_status(void);

#endif
