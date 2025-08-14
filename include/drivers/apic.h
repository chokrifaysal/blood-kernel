/*
 * apic.h – x86 Advanced Programmable Interrupt Controller
 */

#ifndef APIC_H
#define APIC_H

#include "kernel/types.h"

/* Core functions */
void apic_init(u32 lapic_addr, u32 ioapic_addr);
u8 apic_is_enabled(void);

/* Local APIC functions */
void apic_send_eoi(void);
u8 apic_get_id(void);
u32 apic_get_version(void);
u32 apic_get_error_status(void);
void apic_clear_errors(void);

/* Inter-processor interrupts */
void apic_send_ipi(u8 dest_apic_id, u8 vector);
void apic_send_init_ipi(u8 dest_apic_id);
void apic_send_startup_ipi(u8 dest_apic_id, u8 vector);
void apic_broadcast_ipi(u8 vector);

/* Local APIC timer */
void apic_timer_init(u32 frequency);
void apic_timer_stop(void);

/* I/O APIC functions */
void ioapic_init(void);
void ioapic_set_irq(u8 irq, u8 vector, u8 dest_apic_id);
void ioapic_mask_irq(u8 irq);
void ioapic_unmask_irq(u8 irq);

#endif
