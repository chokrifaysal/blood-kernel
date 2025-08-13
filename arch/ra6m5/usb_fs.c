/*
 * usb_fs.c – RA6M5 USB FS OTG device core
 * Minimal control endpoint 0 only
 */

#include "kernel/types.h"

#define USB_BASE 0x40090000UL

typedef volatile struct {
    u32 SYSCFG;
    u32 INTSTS0;
    u32 INTSTS1;
    u32 D0FIFOSEL;
    u32 D0FIFOCTR;
    u32 D1FIFOSEL;
    u32 D1FIFOCTR;
    u32 D0FIFO;
    u32 D1FIFO;
} USB_TypeDef;

static USB_TypeDef* const USB = (USB_TypeDef*)USB_BASE;

void usb_init(void) {
    USB->SYSCFG = (1<<0); // USBE
}

void usb_poll(void) {
    /* bare EP0 read */
}
