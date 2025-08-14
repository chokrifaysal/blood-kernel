/*
 * thermal.h – x86 thermal management and monitoring
 */

#ifndef THERMAL_H
#define THERMAL_H

#include "kernel/types.h"

/* Core functions */
void thermal_init(void);
u8 thermal_is_supported(void);
u8 thermal_has_digital_sensor(void);

/* Temperature monitoring */
u32 thermal_get_temperature(void);
u32 thermal_get_package_temperature(void);
u32 thermal_get_tjmax(void);
u32 thermal_get_max_temperature(void);

/* Status functions */
u8 thermal_is_throttling(void);
u8 thermal_is_critical(void);
u32 thermal_get_event_count(void);

/* Threshold management */
void thermal_set_threshold(u8 threshold_num, u32 temperature);
u32 thermal_get_threshold(u8 threshold_num);

/* Control functions */
void thermal_enable_monitoring(u8 enable);
u8 thermal_is_monitoring_enabled(void);
void thermal_clear_log_bits(void);

/* Interrupt handler */
void thermal_irq_handler(void);

#endif
