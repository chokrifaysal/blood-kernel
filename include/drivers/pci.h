/*
 * pci.h – x86 PCI bus driver
 */

#ifndef PCI_H
#define PCI_H

#include "kernel/types.h"

typedef struct {
    u8 bus;
    u8 device;
    u8 function;
    u16 vendor_id;
    u16 device_id;
    u8 class_code;
    u8 subclass;
    u8 prog_if;
    u8 revision;
    u8 header_type;
    u32 bar[6];
    u8 irq_line;
    u8 irq_pin;
} pci_device_t;

/* Core functions */
void pci_init(void);
u16 pci_get_device_count(void);
pci_device_t* pci_get_device(u16 index);

/* Search functions */
pci_device_t* pci_find_device(u16 vendor_id, u16 device_id);
pci_device_t* pci_find_class(u8 class_code, u8 subclass);

/* Configuration functions */
void pci_enable_device(pci_device_t* dev);
void pci_set_irq_line(pci_device_t* dev, u8 irq);

/* Utility functions */
const char* pci_get_class_name(u8 class_code);

#endif
