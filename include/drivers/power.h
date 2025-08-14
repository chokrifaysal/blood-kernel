/*
 * power.h – x86 CPU power management (P-states, C-states)
 */

#ifndef POWER_H
#define POWER_H

#include "kernel/types.h"

/* Energy performance bias values */
#define EPB_PERFORMANCE         0x00
#define EPB_BALANCED_PERF       0x04
#define EPB_BALANCED            0x06
#define EPB_BALANCED_POWER      0x08
#define EPB_POWER_SAVE          0x0F

/* Core functions */
void power_init(void);
u8 power_is_pstate_supported(void);
u8 power_is_cstate_supported(void);

/* P-state management */
u8 power_set_pstate(u8 pstate);
u8 power_get_pstate(void);
void power_set_frequency(u16 frequency_mhz);
u16 power_get_frequency(void);
u8 power_get_max_pstate(void);
u8 power_get_min_pstate(void);
u8 power_get_num_pstates(void);

/* Turbo boost */
void power_enable_turbo(u8 enable);
u8 power_is_turbo_enabled(void);

/* Clock modulation */
void power_set_clock_modulation(u8 duty_cycle);
u8 power_get_clock_modulation(void);

/* Energy performance bias */
void power_set_energy_perf_bias(u8 bias);
u8 power_get_energy_perf_bias(void);

/* C-state management */
void power_enter_cstate(u8 cstate);
u8 power_get_num_cstates(void);

/* Energy monitoring */
u32 power_get_package_energy(void);
u32 power_get_dram_energy(void);

#endif
