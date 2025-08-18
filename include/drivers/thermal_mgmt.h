/*
 * thermal_mgmt.h – x86 CPU thermal management (thermal throttling/RAPL)
 */

#ifndef THERMAL_MGMT_H
#define THERMAL_MGMT_H

#include "kernel/types.h"

/* Thermal event types */
#define THERMAL_EVENT_THRESHOLD         0
#define THERMAL_EVENT_PROCHOT           1
#define THERMAL_EVENT_CRITICAL          2
#define THERMAL_EVENT_POWER_LIMIT       3

typedef struct {
    u8 event_type;
    u64 timestamp;
    u32 temperature;
    u32 cpu_id;
    u32 duration_ms;
} thermal_event_t;

typedef struct {
    u32 power_units;
    u32 energy_units;
    u32 time_units;
    u64 pkg_power_limit;
    u64 pkg_energy_consumed;
    u64 dram_energy_consumed;
    u64 pp0_energy_consumed;
    u64 pp1_energy_consumed;
    u32 pkg_power_limit_1;
    u32 pkg_power_limit_2;
    u32 pkg_time_window_1;
    u32 pkg_time_window_2;
    u8 pkg_power_limit_enabled;
    u8 dram_power_limit_enabled;
} rapl_info_t;

/* Core functions */
void thermal_mgmt_init(void);

/* Support detection */
u8 thermal_mgmt_is_supported(void);
u8 thermal_mgmt_is_thermal_monitoring_supported(void);
u8 thermal_mgmt_is_rapl_supported(void);

/* Temperature monitoring */
u32 thermal_mgmt_read_temperature(void);
u32 thermal_mgmt_get_current_temperature(void);
u32 thermal_mgmt_get_max_temperature(void);
u32 thermal_mgmt_get_critical_temperature(void);
u32 thermal_mgmt_get_thermal_target(void);

/* Thermal status */
u8 thermal_mgmt_is_thermal_throttling_active(void);
u8 thermal_mgmt_is_prochot_active(void);
u8 thermal_mgmt_is_critical_temp_reached(void);

/* Thermal control */
u8 thermal_mgmt_enable_thermal_interrupt(u32 threshold_temperature);

/* Power management (RAPL) */
u8 thermal_mgmt_set_power_limit(u32 power_limit_watts, u32 time_window_seconds);
u64 thermal_mgmt_read_energy_consumption(u8 domain);
const rapl_info_t* thermal_mgmt_get_rapl_info(void);

/* Event handling */
void thermal_mgmt_handle_thermal_event(u8 event_type, u32 temperature);
u32 thermal_mgmt_get_thermal_events_count(void);
const thermal_event_t* thermal_mgmt_get_thermal_event(u32 index);

/* Statistics */
u32 thermal_mgmt_get_throttle_count(void);
u64 thermal_mgmt_get_total_throttle_time(void);
u64 thermal_mgmt_get_last_thermal_event(void);
u32 thermal_mgmt_get_thermal_interrupts(void);
u32 thermal_mgmt_get_power_limit_violations(void);

/* Utilities */
const char* thermal_mgmt_get_thermal_event_name(u8 event_type);
void thermal_mgmt_clear_statistics(void);

#endif
