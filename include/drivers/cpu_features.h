/*
 * cpu_features.h – x86 CPU feature control and model-specific management
 */

#ifndef CPU_FEATURES_H
#define CPU_FEATURES_H

#include "kernel/types.h"

/* Core functions */
void cpu_features_init(void);

/* Vendor detection */
u8 cpu_features_is_intel(void);
u8 cpu_features_is_amd(void);

/* VMX (Virtualization) control */
u8 cpu_features_is_vmx_supported(void);
u8 cpu_features_is_vmx_locked(void);
u8 cpu_features_is_smx_supported(void);
u8 cpu_features_enable_vmx(void);

/* SpeedStep control */
u8 cpu_features_is_speedstep_enabled(void);
u8 cpu_features_enable_speedstep(void);
u8 cpu_features_disable_speedstep(void);

/* Turbo boost control */
u8 cpu_features_is_turbo_enabled(void);
u8 cpu_features_enable_turbo(void);
u8 cpu_features_disable_turbo(void);

/* Execute Disable control */
u8 cpu_features_is_xd_enabled(void);
u8 cpu_features_enable_xd(void);
u8 cpu_features_disable_xd(void);

/* Feature status */
u8 cpu_features_is_fast_strings_enabled(void);
u8 cpu_features_is_thermal_control_enabled(void);

/* Performance information */
u32 cpu_features_get_max_non_turbo_ratio(void);
u32 cpu_features_get_max_turbo_ratio(void);

/* Capabilities */
u64 cpu_features_get_arch_capabilities(void);
u64 cpu_features_get_core_capabilities(void);

/* Energy performance bias */
u8 cpu_features_is_energy_perf_bias_supported(void);
u8 cpu_features_set_energy_perf_bias(u8 bias);
u8 cpu_features_get_energy_perf_bias(void);

/* MSR access */
u64 cpu_features_get_misc_enable(void);
u64 cpu_features_get_feature_control(void);

/* CPUID control */
u8 cpu_features_limit_cpuid_maxval(u8 enable);

#endif
