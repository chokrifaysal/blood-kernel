/*
 * usb_hs.c – USB HS OTG device core
 */

#include "kernel/types.h"

#define USBHS_BASE 0x40200000UL

typedef volatile struct {
    u32 OTGSC;
    u32 USBSTS;
    u32 USBINTR;
    u32 ENDPTLISTADDR;
    u32 PORTSC1;
} USBHS_TypeDef;

static USBHS_TypeDef* const USBHS = (USBHS_TypeDef*)USBHS_BASE;

void usb_hs_init(void) {
    USBHS->OTGSC = (1<<3); // VBUS discharge
}

void usb_hs_poll(void) {
    /* minimal EP0 handling */
}
