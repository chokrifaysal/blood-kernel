/*
 * cpufreq.h – x86 CPU frequency scaling (SpeedStep/Cool'n'Quiet)
 */

#ifndef CPUFREQ_H
#define CPUFREQ_H

#include "kernel/types.h"

/* Governor policies */
#define CPUFREQ_POLICY_PERFORMANCE  0
#define CPUFREQ_POLICY_POWERSAVE     1
#define CPUFREQ_POLICY_ONDEMAND      2
#define CPUFREQ_POLICY_CONSERVATIVE  3
#define CPUFREQ_POLICY_USERSPACE     4

/* Core functions */
void cpufreq_init(void);

/* Support detection */
u8 cpufreq_is_supported(void);
u8 cpufreq_is_intel_speedstep(void);
u8 cpufreq_is_amd_coolnquiet(void);

/* Turbo boost control */
u8 cpufreq_is_turbo_supported(void);
u8 cpufreq_is_turbo_enabled(void);
void cpufreq_enable_turbo(void);
void cpufreq_disable_turbo(void);

/* P-state management */
u8 cpufreq_set_pstate(u8 pstate);
u8 cpufreq_get_current_pstate(void);
u8 cpufreq_get_num_pstates(void);

/* Frequency control */
u16 cpufreq_get_current_frequency(void);
u16 cpufreq_get_pstate_frequency(u8 pstate);
u16 cpufreq_get_base_frequency(void);
u16 cpufreq_get_max_turbo_frequency(void);
void cpufreq_set_frequency(u16 target_freq);

/* P-state information */
u8 cpufreq_get_pstate_voltage(u8 pstate);
u8 cpufreq_get_pstate_power(u8 pstate);
u32 cpufreq_get_transition_latency(void);

/* Governor control */
void cpufreq_set_governor_policy(u8 policy);
u8 cpufreq_get_governor_policy(void);

#endif
