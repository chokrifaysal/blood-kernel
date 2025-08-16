/*
 * ioapic.h – x86 I/O Advanced Programmable Interrupt Controller
 */

#ifndef IOAPIC_H
#define IOAPIC_H

#include "kernel/types.h"

/* Delivery modes */
#define IOAPIC_DELIVERY_FIXED   0x00
#define IOAPIC_DELIVERY_LOWEST  0x01
#define IOAPIC_DELIVERY_SMI     0x02
#define IOAPIC_DELIVERY_NMI     0x04
#define IOAPIC_DELIVERY_INIT    0x05
#define IOAPIC_DELIVERY_EXTINT  0x07

/* Core functions */
void ioapic_init(void);

/* Status functions */
u8 ioapic_is_initialized(void);
u8 ioapic_get_num_controllers(void);

/* Controller information */
u32 ioapic_get_controller_base(u8 controller_id);
u8 ioapic_get_controller_id(u8 controller_id);
u8 ioapic_get_max_redirection_entries(u8 controller_id);
u32 ioapic_get_gsi_base(u8 controller_id);

/* IRQ management */
void ioapic_set_irq(u32 gsi, u8 vector, u8 dest_apic_id, u8 trigger_mode, u8 polarity);
void ioapic_mask_irq(u32 gsi);
void ioapic_unmask_irq(u32 gsi);
u8 ioapic_is_irq_masked(u32 gsi);

/* Advanced configuration */
void ioapic_set_delivery_mode(u32 gsi, u8 delivery_mode);
void ioapic_set_destination_mode(u32 gsi, u8 logical_mode);
u8 ioapic_get_vector(u32 gsi);
u8 ioapic_get_destination(u32 gsi);

/* Legacy support */
void ioapic_disable_legacy_pic(void);
void ioapic_setup_legacy_irqs(void);

#endif
