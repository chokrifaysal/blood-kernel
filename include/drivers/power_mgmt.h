/*
 * power_mgmt.h – x86 advanced power management (C-states/P-states deep control)
 */

#ifndef POWER_MGMT_H
#define POWER_MGMT_H

#include "kernel/types.h"

/* ACPI C-state types */
#define CSTATE_C0           0   /* Active */
#define CSTATE_C1           1   /* Halt */
#define CSTATE_C1E          2   /* Enhanced Halt */
#define CSTATE_C2           3   /* Stop Clock */
#define CSTATE_C3           4   /* Sleep */
#define CSTATE_C4           5   /* Deeper Sleep */
#define CSTATE_C6           6   /* Deep Power Down */
#define CSTATE_C7           7   /* Deeper Power Down */
#define CSTATE_C8           8   /* Deepest Power Down */

typedef struct {
    u8 type;
    u8 latency;
    u16 power;
    u32 mwait_hint;
    u64 residency_counter;
    u32 usage_count;
} cstate_info_t;

/* Core functions */
void power_mgmt_init(void);

/* Support detection */
u8 power_mgmt_is_supported(void);
u8 power_mgmt_is_cstate_supported(void);
u8 power_mgmt_is_energy_monitoring_supported(void);

/* C-state management */
u8 power_mgmt_get_num_cstates(void);
u8 power_mgmt_get_deepest_cstate(void);
u8 power_mgmt_get_current_cstate(void);
const cstate_info_t* power_mgmt_get_cstate_info(u8 cstate);
void power_mgmt_enter_cstate(u8 cstate);
void power_mgmt_idle(void);

/* C-state statistics */
u64 power_mgmt_get_cstate_residency(u8 cstate);
u64 power_mgmt_get_package_cstate_residency(u8 cstate);
u32 power_mgmt_get_cstate_usage_count(u8 cstate);

/* Energy monitoring */
u64 power_mgmt_get_energy_consumed(void);
u32 power_mgmt_get_current_power(void);
u32 power_mgmt_get_thermal_design_power(void);
u32 power_mgmt_get_minimum_power(void);
u32 power_mgmt_get_maximum_power(void);

/* Power limiting */
void power_mgmt_set_power_limit(u32 power_limit_watts);
u32 power_mgmt_get_power_limit(void);

/* Package C-state control */
void power_mgmt_enable_package_cstates(void);
void power_mgmt_disable_package_cstates(void);
u8 power_mgmt_are_package_cstates_enabled(void);

#endif
