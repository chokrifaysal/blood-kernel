/*
 * usb_hs.c – STM32H745 USB HS OTG device mode
 */

#include "kernel/types.h"

#define USB_OTG_HS_BASE 0x40040000UL
#define RCC_AHB1ENR (*(volatile u32*)0x58024530UL)

typedef volatile struct {
    u32 GOTGCTL;
    u32 GOTGINT;
    u32 GAHBCFG;
    u32 GUSBCFG;
    u32 GRSTCTL;
    u32 GINTSTS;
    u32 GINTMSK;
    u32 GRXSTSR;
    u32 GRXSTSP;
    u32 GRXFSIZ;
    u32 GNPTXFSIZ;
    u32 GNPTXSTS;
    u32 GI2CCTL;
    u32 _pad1;
    u32 GCCFG;
    u32 CID;
    u32 GSNPSID;
    u32 GHWCFG1;
    u32 GHWCFG2;
    u32 GHWCFG3;
    u32 _pad2;
    u32 GLPMCFG;
    u32 GPWRDN;
    u32 GDFIFOCFG;
    u32 GADPCTL;
    u32 _pad3[39];
    u32 HPTXFSIZ;
    u32 DIEPTXF[15];
} USB_OTG_GlobalTypeDef;

typedef volatile struct {
    u32 DCFG;
    u32 DCTL;
    u32 DSTS;
    u32 _pad1;
    u32 DIEPMSK;
    u32 DOEPMSK;
    u32 DAINT;
    u32 DAINTMSK;
    u32 _pad2[2];
    u32 DVBUSDIS;
    u32 DVBUSPULSE;
    u32 DTHRCTL;
    u32 DIEPEMPMSK;
    u32 DEACHINT;
    u32 DEACHMSK;
} USB_OTG_DeviceTypeDef;

static USB_OTG_GlobalTypeDef* const USB_OTG_HS = (USB_OTG_GlobalTypeDef*)USB_OTG_HS_BASE;
static USB_OTG_DeviceTypeDef* const USB_OTG_HS_DEVICE = (USB_OTG_DeviceTypeDef*)(USB_OTG_HS_BASE + 0x800);

static u8 usb_config = 0;

void usb_hs_init(void) {
    /* Enable USB HS clock */
    RCC_AHB1ENR |= (1<<25);
    
    /* Core soft reset */
    USB_OTG_HS->GRSTCTL = (1<<0);
    while (USB_OTG_HS->GRSTCTL & (1<<0));
    
    /* Wait for AHB idle */
    while (!(USB_OTG_HS->GRSTCTL & (1<<31)));
    
    /* Force device mode */
    USB_OTG_HS->GUSBCFG |= (1<<30);
    
    /* Device configuration */
    USB_OTG_HS_DEVICE->DCFG = (3<<0); // HS/FS
    
    /* Enable VBUS sensing */
    USB_OTG_HS->GCCFG |= (1<<19) | (1<<16);
    
    /* Unmask interrupts */
    USB_OTG_HS->GINTMSK = (1<<12) | (1<<11) | (1<<4) | (1<<3);
    
    /* Global interrupt enable */
    USB_OTG_HS->GAHBCFG |= (1<<0);
}

void usb_hs_connect(void) {
    USB_OTG_HS_DEVICE->DCTL &= ~(1<<1); // Clear soft disconnect
}

void usb_hs_disconnect(void) {
    USB_OTG_HS_DEVICE->DCTL |= (1<<1); // Soft disconnect
}

u8 usb_hs_configured(void) {
    return usb_config;
}

void usb_hs_set_address(u8 addr) {
    USB_OTG_HS_DEVICE->DCFG = (USB_OTG_HS_DEVICE->DCFG & ~(0x7F<<4)) | (addr<<4);
}

void usb_hs_ep0_send(const u8* data, u16 len) {
    /* Setup EP0 IN */
    volatile u32* DIEPCTL0 = (u32*)(USB_OTG_HS_BASE + 0x900);
    volatile u32* DIEPTSIZ0 = (u32*)(USB_OTG_HS_BASE + 0x910);
    volatile u32* DTXFSTS0 = (u32*)(USB_OTG_HS_BASE + 0x918);
    
    *DIEPTSIZ0 = (1<<19) | len; // 1 packet, len bytes
    *DIEPCTL0 |= (1<<31) | (1<<26); // Enable + clear NAK
    
    /* Write data to FIFO */
    volatile u32* FIFO0 = (u32*)(USB_OTG_HS_BASE + 0x1000);
    for (u16 i = 0; i < (len + 3) / 4; i++) {
        u32 word = 0;
        for (u8 j = 0; j < 4 && i*4+j < len; j++) {
            word |= data[i*4+j] << (j*8);
        }
        *FIFO0 = word;
    }
}

void usb_hs_ep0_recv(u8* data, u16 len) {
    /* Setup EP0 OUT */
    volatile u32* DOEPCTL0 = (u32*)(USB_OTG_HS_BASE + 0xB00);
    volatile u32* DOEPTSIZ0 = (u32*)(USB_OTG_HS_BASE + 0xB10);
    
    *DOEPTSIZ0 = (1<<29) | (1<<19) | len; // Setup count=1, packet count=1
    *DOEPCTL0 |= (1<<31) | (1<<26); // Enable + clear NAK
}

void usb_hs_irq_handler(void) {
    u32 gintsts = USB_OTG_HS->GINTSTS;
    
    if (gintsts & (1<<12)) { // Reset
        usb_config = 0;
        USB_OTG_HS->GINTSTS = (1<<12);
    }
    
    if (gintsts & (1<<11)) { // Enumeration done
        USB_OTG_HS->GINTSTS = (1<<11);
    }
    
    if (gintsts & (1<<4)) { // RX FIFO non-empty
        u32 grxsts = USB_OTG_HS->GRXSTSP;
        u8 epnum = grxsts & 0xF;
        u16 bcnt = (grxsts >> 4) & 0x7FF;
        u8 pktsts = (grxsts >> 17) & 0xF;
        
        if (pktsts == 2) { // OUT received
            // Handle OUT data
        } else if (pktsts == 6) { // Setup received
            // Handle SETUP packet
        }
    }
}
