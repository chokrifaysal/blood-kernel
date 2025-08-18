/*
 * power_states.h – x86 CPU power state management (C-states/P-states extensions)
 */

#ifndef POWER_STATES_H
#define POWER_STATES_H

#include "kernel/types.h"

/* C-state definitions */
#define CSTATE_C0                       0
#define CSTATE_C1                       1
#define CSTATE_C1E                      2
#define CSTATE_C3                       3
#define CSTATE_C6                       6
#define CSTATE_C7                       7
#define CSTATE_C8                       8
#define CSTATE_C9                       9
#define CSTATE_C10                      10

/* P-state definitions */
#define PSTATE_P0                       0  /* Maximum performance */
#define PSTATE_P1                       1
#define PSTATE_P2                       2
#define PSTATE_P3                       3
#define PSTATE_P4                       4
#define PSTATE_P5                       5
#define PSTATE_P6                       6
#define PSTATE_P7                       7  /* Minimum performance */

typedef struct {
    u8 state;
    u64 entry_time;
    u64 exit_time;
    u64 residency_time;
    u32 entry_count;
    u32 exit_latency_us;
    u8 available;
} cstate_info_t;

typedef struct {
    u8 state;
    u16 frequency_mhz;
    u16 voltage_mv;
    u32 power_mw;
    u64 transition_time;
    u32 transition_count;
    u32 transition_latency_us;
    u8 available;
} pstate_info_t;

/* Core functions */
void power_states_init(void);

/* Support detection */
u8 power_states_is_supported(void);
u8 power_states_is_cstates_supported(void);
u8 power_states_is_pstates_supported(void);

/* C-state operations */
u8 power_states_enter_cstate(u8 cstate);
u8 power_states_get_current_cstate(void);
u8 power_states_get_max_cstate(void);
u8 power_states_get_num_cstates(void);
const cstate_info_t* power_states_get_cstate_info(u8 cstate);
u64 power_states_get_cstate_residency(u8 cstate);

/* P-state operations */
u8 power_states_set_pstate(u8 pstate);
u8 power_states_get_current_pstate(void);
u8 power_states_get_max_pstate(void);
u8 power_states_get_num_pstates(void);
const pstate_info_t* power_states_get_pstate_info(u8 pstate);

/* Thermal management */
u16 power_states_get_current_temperature(void);
u16 power_states_get_max_temperature(void);
u8 power_states_is_thermal_throttling(void);

/* Statistics */
u64 power_states_get_total_transitions(void);
u64 power_states_get_total_cstate_residency(void);
u32 power_states_get_thermal_throttle_count(void);
u32 power_states_get_power_limit_throttle_count(void);
u64 power_states_get_last_transition_time(void);

/* Power management control */
void power_states_enable_power_management(void);
void power_states_disable_power_management(void);
u8 power_states_is_power_management_enabled(void);

/* Utilities */
void power_states_clear_statistics(void);
const char* power_states_get_cstate_name(u8 cstate);
const char* power_states_get_pstate_name(u8 pstate);

#endif
