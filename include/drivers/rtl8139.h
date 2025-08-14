/*
 * rtl8139.h – Realtek RTL8139 Fast Ethernet controller
 */

#ifndef RTL8139_H
#define RTL8139_H

#include "kernel/types.h"

/* Core functions */
u8 rtl8139_init(u16 io_base, u8 irq);
u8 rtl8139_is_initialized(void);

/* Network functions */
u8 rtl8139_send_packet(const void* data, u16 length);
u16 rtl8139_receive_packet(void* buffer, u16 max_length);

/* Status functions */
void rtl8139_get_mac_address(u8* mac);
u8 rtl8139_is_link_up(void);

/* Interrupt handler */
void rtl8139_irq_handler(void);

#endif
