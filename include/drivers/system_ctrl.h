/*
 * system_ctrl.h – x86 advanced system control (SMM/ACPI extensions)
 */

#ifndef SYSTEM_CTRL_H
#define SYSTEM_CTRL_H

#include "kernel/types.h"

/* System reset methods */
#define RESET_METHOD_KBC            0
#define RESET_METHOD_ACPI           1
#define RESET_METHOD_PCI            2
#define RESET_METHOD_CF9            3

/* Power states */
#define ACPI_S0_WORKING             0
#define ACPI_S1_SLEEPING            1
#define ACPI_S3_SUSPEND_TO_RAM      3
#define ACPI_S4_SUSPEND_TO_DISK     4
#define ACPI_S5_SOFT_OFF            5

/* Core functions */
void system_ctrl_init(void);

/* Support detection */
u8 system_ctrl_is_supported(void);
u8 system_ctrl_is_smm_supported(void);
u8 system_ctrl_is_acpi_supported(void);

/* System control */
void system_ctrl_reset(u8 method);
void system_ctrl_shutdown(void);
void system_ctrl_suspend_to_ram(void);
void system_ctrl_suspend_to_disk(void);

/* SMM information */
void system_ctrl_handle_smi(void);
u32 system_ctrl_get_smi_count(void);
u64 system_ctrl_get_last_smi_time(void);
u64 system_ctrl_get_smm_base(void);
u32 system_ctrl_get_smm_size(void);
u8 system_ctrl_is_smm_enabled(void);

/* ACPI power states */
u8 system_ctrl_get_acpi_state(void);
u8 system_ctrl_is_s3_supported(void);
u8 system_ctrl_is_s4_supported(void);
u8 system_ctrl_is_s5_supported(void);

/* Statistics */
u32 system_ctrl_get_reset_count(void);
u32 system_ctrl_get_power_state_changes(void);
u64 system_ctrl_get_uptime(void);
u64 system_ctrl_get_last_reset_time(void);

/* Reset configuration */
void system_ctrl_set_preferred_reset_method(u8 method);
u8 system_ctrl_get_preferred_reset_method(void);

/* Emergency functions */
void system_ctrl_emergency_reset(void);
void system_ctrl_warm_reset(void);
void system_ctrl_cold_reset(void);

/* Reset cause detection */
u8 system_ctrl_detect_reset_cause(void);

/* Wake event management */
void system_ctrl_enable_wake_events(u32 events);
void system_ctrl_disable_wake_events(u32 events);

#endif
