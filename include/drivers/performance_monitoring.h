/*
 * performance_monitoring.h – x86 CPU performance monitoring (PMC/PMU)
 */

#ifndef PERFORMANCE_MONITORING_H
#define PERFORMANCE_MONITORING_H

#include "kernel/types.h"

/* Common performance events */
#define EVENT_UNHALTED_CORE_CYCLES      0x3C
#define EVENT_INSTRUCTION_RETIRED       0xC0
#define EVENT_UNHALTED_REF_CYCLES       0x3C
#define EVENT_LLC_REFERENCES            0x2E
#define EVENT_LLC_MISSES                0x2E
#define EVENT_BRANCH_INSTRUCTIONS       0xC4
#define EVENT_BRANCH_MISSES             0xC5
#define EVENT_CACHE_REFERENCES          0x2E
#define EVENT_CACHE_MISSES              0x2E
#define EVENT_BUS_CYCLES                0x3C
#define EVENT_STALLED_CYCLES_FRONTEND   0x9C
#define EVENT_STALLED_CYCLES_BACKEND    0xA1
#define EVENT_REF_CPU_CYCLES            0x3C

/* UMASK values for events */
#define UMASK_LLC_REFERENCES            0x4F
#define UMASK_LLC_MISSES                0x41
#define UMASK_L1D_REPLACEMENT           0x01
#define UMASK_L1I_REPLACEMENT           0x02
#define UMASK_DTLB_LOAD_MISSES          0x01
#define UMASK_ITLB_MISSES               0x01

typedef struct {
    u32 event_select;
    u32 umask;
    u8 user_mode;
    u8 kernel_mode;
    u8 edge_detect;
    u8 pin_control;
    u8 interrupt_enable;
    u8 any_thread;
    u8 enable;
    u8 invert;
    u32 counter_mask;
} perf_event_config_t;

typedef struct {
    u64 value;
    u64 last_value;
    u64 overflow_count;
    u8 enabled;
    perf_event_config_t config;
} perf_counter_t;

/* Core functions */
void performance_monitoring_init(void);

/* Support detection */
u8 performance_monitoring_is_supported(void);
u8 performance_monitoring_get_version(void);
u8 performance_monitoring_get_num_gp_counters(void);
u8 performance_monitoring_get_gp_counter_width(void);
u8 performance_monitoring_get_num_fixed_counters(void);
u8 performance_monitoring_get_fixed_counter_width(void);

/* Counter configuration */
u8 performance_monitoring_setup_gp_counter(u8 counter, u32 event, u32 umask, u8 user, u8 kernel);
u8 performance_monitoring_enable_fixed_counter(u8 counter, u8 user, u8 kernel);

/* Counter reading */
u64 performance_monitoring_read_gp_counter(u8 counter);
u64 performance_monitoring_read_fixed_counter(u8 counter);

/* Global control */
void performance_monitoring_start_global(void);
void performance_monitoring_stop_global(void);

/* Fixed counter shortcuts */
u64 performance_monitoring_get_instructions_retired(void);
u64 performance_monitoring_get_cpu_cycles(void);
u64 performance_monitoring_get_reference_cycles(void);

/* Performance metrics */
u32 performance_monitoring_calculate_ipc(void);

/* Sampling and overflow handling */
void performance_monitoring_sample_all_counters(void);
void performance_monitoring_handle_overflow(u8 counter);

/* Status and control */
u64 performance_monitoring_get_global_status(void);
void performance_monitoring_clear_global_status(void);

/* Counter information */
const perf_counter_t* performance_monitoring_get_gp_counter_info(u8 counter);
const perf_counter_t* performance_monitoring_get_fixed_counter_info(u8 counter);

/* Statistics */
u32 performance_monitoring_get_total_samples(void);
u32 performance_monitoring_get_overflow_interrupts(void);
u64 performance_monitoring_get_last_sample_time(void);
u64 performance_monitoring_get_total_monitoring_time(void);

/* Utilities */
void performance_monitoring_clear_statistics(void);

#endif
