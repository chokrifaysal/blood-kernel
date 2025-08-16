/*
 * x2apic.h – x86 x2APIC advanced interrupt controller
 */

#ifndef X2APIC_H
#define X2APIC_H

#include "kernel/types.h"

/* Delivery modes */
#define X2APIC_DELIVERY_FIXED      0x000
#define X2APIC_DELIVERY_LOWEST     0x100
#define X2APIC_DELIVERY_SMI        0x200
#define X2APIC_DELIVERY_NMI        0x400
#define X2APIC_DELIVERY_INIT       0x500
#define X2APIC_DELIVERY_STARTUP    0x600

/* Core functions */
void x2apic_init(void);

/* Status functions */
u8 x2apic_is_supported(void);
u8 x2apic_is_enabled(void);
u32 x2apic_get_id(void);
u32 x2apic_get_version(void);
u32 x2apic_get_max_lvt_entries(void);
u8 x2apic_supports_eoi_broadcast_suppression(void);

/* Interrupt handling */
void x2apic_send_eoi(void);
void x2apic_send_ipi(u32 dest_apic_id, u32 vector, u32 delivery_mode);
void x2apic_send_ipi_all_excluding_self(u32 vector, u32 delivery_mode);
void x2apic_send_ipi_self(u32 vector);

/* Timer functions */
void x2apic_setup_timer(u32 vector, u32 initial_count, u8 periodic);
void x2apic_stop_timer(void);
u32 x2apic_get_timer_count(void);
void x2apic_calibrate_timer(void);
u32 x2apic_get_timer_frequency(void);
u8 x2apic_is_timer_running(void);

/* Local interrupt setup */
void x2apic_setup_lint(u8 lint_num, u32 vector, u32 delivery_mode, u8 trigger_mode, u8 polarity);
void x2apic_mask_lint(u8 lint_num);
void x2apic_unmask_lint(u8 lint_num);

/* Error handling */
u32 x2apic_get_error_status(void);
void x2apic_clear_error_status(void);

#endif
