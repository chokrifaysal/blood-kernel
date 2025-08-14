/*
 * usb_uhci.h – x86 USB Universal Host Controller Interface
 */

#ifndef USB_UHCI_H
#define USB_UHCI_H

#include "kernel/types.h"

/* Core functions */
u8 uhci_init(u16 io_base, u8 irq);
u8 uhci_get_controller_count(void);

/* Port management */
void uhci_port_reset(u8 controller, u8 port);
u16 uhci_get_port_status(u8 controller, u8 port);

/* Transfer functions */
u8 uhci_control_transfer(u8 controller, u8 device_addr, u8 endpoint, 
                        const void* setup_packet, void* data, u16 data_len);

/* Interrupt handler */
void uhci_irq_handler(u8 controller);

/* Port status bits */
#define UHCI_PORT_CONNECTION    0x0001
#define UHCI_PORT_ENABLED       0x0004
#define UHCI_PORT_LOW_SPEED     0x0100
#define UHCI_PORT_RESET         0x0200

#endif
