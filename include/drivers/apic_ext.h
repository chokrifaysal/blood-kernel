/*
 * apic_ext.h – x86 advanced interrupt handling (APIC extensions/IPI)
 */

#ifndef APIC_EXT_H
#define APIC_EXT_H

#include "kernel/types.h"

/* ICR delivery modes */
#define ICR_DELIVERY_FIXED              0x0
#define ICR_DELIVERY_LOWEST             0x1
#define ICR_DELIVERY_SMI                0x2
#define ICR_DELIVERY_NMI                0x4
#define ICR_DELIVERY_INIT               0x5
#define ICR_DELIVERY_STARTUP            0x6

/* Timer modes */
#define APIC_TIMER_ONESHOT              0x0
#define APIC_TIMER_PERIODIC             0x1
#define APIC_TIMER_TSC_DEADLINE         0x2

typedef struct {
    u32 vector;
    u32 delivery_mode;
    u32 dest_mode;
    u32 dest_shorthand;
    u32 destination;
    u64 timestamp;
    u8 delivered;
} ipi_record_t;

/* Core functions */
void apic_ext_init(void);

/* Support detection */
u8 apic_ext_is_supported(void);
u8 apic_ext_is_x2apic_supported(void);
u8 apic_ext_is_x2apic_enabled(void);

/* x2APIC control */
u8 apic_ext_enable_x2apic(void);

/* APIC information */
u32 apic_ext_get_id(void);
u32 apic_ext_get_version(void);
u8 apic_ext_get_max_lvt_entries(void);

/* IPI operations */
void apic_ext_send_ipi(u32 dest_apic_id, u32 vector, u32 delivery_mode);
void apic_ext_send_ipi_broadcast(u32 vector, u32 delivery_mode, u8 include_self);
void apic_ext_send_nmi(u32 dest_apic_id);
void apic_ext_send_init(u32 dest_apic_id);
void apic_ext_send_startup(u32 dest_apic_id, u32 start_page);

/* Interrupt handling */
void apic_ext_eoi(void);

/* Interrupt status */
u32 apic_ext_get_isr(u8 reg);
u32 apic_ext_get_irr(u8 reg);
u32 apic_ext_get_tmr(u8 reg);

/* Timer operations */
void apic_ext_setup_timer(u32 initial_count, u8 mode, u8 vector);
u32 apic_ext_get_timer_current_count(void);

/* Priority control */
void apic_ext_set_task_priority(u8 priority);
u8 apic_ext_get_task_priority(void);

/* Error handling */
u32 apic_ext_get_error_status(void);

/* Statistics */
u32 apic_ext_get_ipi_count(void);
u32 apic_ext_get_broadcast_count(void);
u32 apic_ext_get_nmi_count(void);
u32 apic_ext_get_init_count(void);
u32 apic_ext_get_startup_count(void);
const ipi_record_t* apic_ext_get_ipi_history(u32 index);
u32 apic_ext_get_error_count(void);
u32 apic_ext_get_last_error(void);
u64 apic_ext_get_last_ipi_time(void);

/* Utilities */
void apic_ext_clear_statistics(void);

#endif
