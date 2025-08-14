/*
 * acpi.h – x86 Advanced Configuration and Power Interface
 */

#ifndef ACPI_H
#define ACPI_H

#include "kernel/types.h"

/* Core functions */
void acpi_init(void);
u8 acpi_is_available(void);

/* Power management */
void acpi_enable(void);
u8 acpi_is_enabled(void);
void acpi_power_off(void);
void acpi_reboot(void);

/* Hardware information */
u32 acpi_get_local_apic_base(void);
u8 acpi_get_cpu_count(void);
u32 acpi_get_pm_timer(void);

/* MADT enumeration */
void acpi_enumerate_io_apics(void (*callback)(u8 id, u32 address, u32 gsi_base));
void acpi_enumerate_interrupt_overrides(void (*callback)(u8 source, u32 gsi, u16 flags));

#endif
