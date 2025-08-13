/*
 */

#include "kernel/types.h"

#define USB_BASE 0x40005C00UL
#define USB_ISTR (*(volatile u32*)(USB_BASE + 0x0C))
#define USB_CNTR (*(volatile u32*)(USB_BASE + 0x10))
#define USB_EP0R (*(volatile u32*)(USB_BASE + 0x00))
#define USB_EP1R (*(volatile u32*)(USB_BASE + 0x04))
#define USB_DADDR (*(volatile u32*)(USB_BASE + 0x14))

static u8 usb_buf[512];
static u8 usb_state = 0;

static void usb_reset(void) {
    USB_CNTR = (1<<0); // reset
    USB_EP0R = (1<<15); // CTR_RX
}

static void usb_setup(void) {
    USB_CNTR |= (1<<15); // FSUSP
}

static void usb_bulk_send(const u8* data, u16 len) {
    for (u16 i = 0; i < len; ++i) usb_buf[i] = data[i];
    USB_EP1R = len;
}

static void usb_bulk_recv(u8* data, u16 len) {
    for (u16 i = 0; i < len; ++i) data[i] = usb_buf[i];
}

void usb_init(void) {
    usb_reset();
    usb_setup();
}

void usb_poll(void) {
    if (USB_ISTR & (1<<15)) usb_reset();
}
