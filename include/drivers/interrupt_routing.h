/*
 * interrupt_routing.h – x86 advanced interrupt routing (I/O APIC/MSI/MSI-X)
 */

#ifndef INTERRUPT_ROUTING_H
#define INTERRUPT_ROUTING_H

#include "kernel/types.h"

/* Interrupt delivery modes */
#define DELIVERY_MODE_FIXED             0x0
#define DELIVERY_MODE_LOWEST            0x1
#define DELIVERY_MODE_SMI               0x2
#define DELIVERY_MODE_NMI               0x4
#define DELIVERY_MODE_INIT              0x5
#define DELIVERY_MODE_EXTINT            0x7

/* Interrupt trigger modes */
#define TRIGGER_MODE_EDGE               0
#define TRIGGER_MODE_LEVEL              1

/* Interrupt polarity */
#define POLARITY_ACTIVE_HIGH            0
#define POLARITY_ACTIVE_LOW             1

typedef struct {
    u64 address;
    u32 data;
    u8 vector;
    u8 delivery_mode;
    u8 destination;
    u8 enabled;
} msi_entry_t;

typedef struct {
    u64 address;
    u32 data;
    u32 vector_control;
    u8 masked;
} msix_entry_t;

typedef struct {
    u32 vector;
    u8 delivery_mode;
    u8 dest_mode;
    u8 polarity;
    u8 trigger_mode;
    u8 mask;
    u8 destination;
} ioapic_entry_t;

typedef struct {
    u32 base_address;
    u8 id;
    u8 version;
    u8 max_redirection_entries;
    ioapic_entry_t entries[24];
    u32 interrupt_count[24];
} ioapic_info_t;

/* Core functions */
void interrupt_routing_init(void);

/* Support detection */
u8 interrupt_routing_is_supported(void);
u8 interrupt_routing_is_ioapic_supported(void);
u8 interrupt_routing_is_msi_supported(void);
u8 interrupt_routing_is_msix_supported(void);

/* I/O APIC management */
u8 interrupt_routing_get_num_ioapics(void);
const ioapic_info_t* interrupt_routing_get_ioapic_info(u8 ioapic_index);
u8 interrupt_routing_setup_ioapic_entry(u8 ioapic_index, u8 pin, u8 vector, 
                                       u8 delivery_mode, u8 dest_mode, 
                                       u8 polarity, u8 trigger_mode, u8 destination);
u8 interrupt_routing_mask_ioapic_entry(u8 ioapic_index, u8 pin);
u8 interrupt_routing_unmask_ioapic_entry(u8 ioapic_index, u8 pin);

/* MSI management */
u8 interrupt_routing_setup_msi(u8 bus, u8 device, u8 function, u8 vector, u8 destination);
u32 interrupt_routing_get_num_msi_entries(void);
const msi_entry_t* interrupt_routing_get_msi_entry(u32 index);

/* MSI-X management */
u32 interrupt_routing_get_num_msix_entries(void);
const msix_entry_t* interrupt_routing_get_msix_entry(u32 index);

/* Interrupt handling */
void interrupt_routing_handle_interrupt(u8 vector);

/* Statistics */
u32 interrupt_routing_get_total_interrupts_routed(void);
u32 interrupt_routing_get_msi_interrupts(void);
u32 interrupt_routing_get_msix_interrupts(void);
u32 interrupt_routing_get_ioapic_interrupts(void);
u64 interrupt_routing_get_last_interrupt_time(void);

/* Utilities */
void interrupt_routing_clear_statistics(void);
const char* interrupt_routing_get_delivery_mode_name(u8 delivery_mode);

#endif
