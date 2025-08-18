/*
 * timing_sync.h – x86 CPU timing/synchronization (TSC/HPET extensions)
 */

#ifndef TIMING_SYNC_H
#define TIMING_SYNC_H

#include "kernel/types.h"

/* Core functions */
void timing_sync_init(void);

/* Support detection */
u8 timing_sync_is_supported(void);
u8 timing_sync_is_tsc_supported(void);
u8 timing_sync_is_hpet_supported(void);
u8 timing_sync_is_tsc_invariant(void);
u8 timing_sync_is_rdtscp_supported(void);

/* TSC operations */
u64 timing_sync_get_tsc(void);
u64 timing_sync_get_tsc_with_cpu_id(u32* cpu_id);
u64 timing_sync_get_tsc_frequency(void);
void timing_sync_set_tsc_deadline(u64 deadline);

/* HPET operations */
u64 timing_sync_get_hpet_counter(void);
u64 timing_sync_get_hpet_frequency(void);
void timing_sync_enable_hpet(void);
void timing_sync_disable_hpet(void);
void timing_sync_setup_hpet_timer(u8 timer_id, u64 period, u8 periodic, u8 interrupt_vector);

/* Synchronization primitives */
void timing_sync_barrier_init(u32 num_cpus);
void timing_sync_barrier_wait(void);

/* Timing measurements */
u64 timing_sync_measure_latency(void (*function)(void));
u64 timing_sync_cycles_to_ns(u64 cycles);
u64 timing_sync_ns_to_cycles(u64 nanoseconds);

/* Calibration */
void timing_sync_calibrate_tsc(void);

/* Statistics */
u32 timing_sync_get_sync_operations(void);
u32 timing_sync_get_timing_measurements(void);
u64 timing_sync_get_last_sync_time(void);
u64 timing_sync_get_max_sync_latency(void);
u64 timing_sync_get_min_sync_latency(void);

/* HPET information */
u8 timing_sync_get_hpet_num_timers(void);
u8 timing_sync_get_hpet_counter_size(void);

/* Utilities */
void timing_sync_clear_statistics(void);

#endif
