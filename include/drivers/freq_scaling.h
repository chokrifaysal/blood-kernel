/*
 * freq_scaling.h – x86 CPU frequency scaling (P-states/Turbo)
 */

#ifndef FREQ_SCALING_H
#define FREQ_SCALING_H

#include "kernel/types.h"

/* Energy Performance Bias */
#define EPB_PERFORMANCE             0
#define EPB_BALANCED_PERFORMANCE    4
#define EPB_BALANCED_POWER          8
#define EPB_POWER_SAVE              15

typedef struct {
    u16 frequency_mhz;
    u8 ratio;
    u8 voltage;
    u32 power_mw;
    u8 enabled;
    u32 transition_latency_us;
} pstate_info_t;

/* Core functions */
void freq_scaling_init(void);

/* Support detection */
u8 freq_scaling_is_supported(void);
u8 freq_scaling_is_turbo_supported(void);
u8 freq_scaling_is_speedstep_supported(void);

/* Frequency control */
u16 freq_scaling_get_current_frequency(void);
u8 freq_scaling_set_frequency(u16 frequency_mhz);
u8 freq_scaling_set_pstate(u8 pstate);

/* Turbo control */
void freq_scaling_enable_turbo(void);
void freq_scaling_disable_turbo(void);
u8 freq_scaling_is_turbo_enabled(void);

/* Energy performance bias */
void freq_scaling_set_energy_perf_bias(u8 bias);
u8 freq_scaling_get_energy_perf_bias(void);

/* Information */
u16 freq_scaling_get_max_frequency(void);
u16 freq_scaling_get_base_frequency(void);
u8 freq_scaling_get_num_pstates(void);
const pstate_info_t* freq_scaling_get_pstate_info(u8 pstate);

/* Statistics */
u32 freq_scaling_get_transition_count(void);
u32 freq_scaling_get_turbo_activations(void);
u64 freq_scaling_get_last_transition_time(void);

/* Preset modes */
void freq_scaling_set_performance_mode(void);
void freq_scaling_set_power_save_mode(void);
void freq_scaling_set_balanced_mode(void);

/* Status */
u8 freq_scaling_get_current_pstate(void);
u8 freq_scaling_is_thermal_throttling(void);
u8 freq_scaling_is_power_limit_throttling(void);

#endif
