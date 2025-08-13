/*
 * usb_fs.c – GD32VF103 USB-FS host stack
 * Control + bulk, 48 MHz PHY, no hub, no OTG
 */

#include "kernel/types.h"

#define USB_BASE           0x40005C00UL
#define USB_ISTR           (*(volatile u32*)(USB_BASE + 0x0C))
#define USB_CNTR           (*(volatile u32*)(USB_BASE + 0x10))
#define USB_FNR            (*(volatile u32*)(USB_BASE + 0x14))
#define USB_DADDR          (*(volatile u32*)(USB_BASE + 0x18))
#define USB_BTABLE         (*(volatile u32*)(USB_BASE + 0x1C))

#define EP0_DESC           0x40006000UL
#define EP1_DESC           0x40006008UL
#define EP0_TX_ADDR        0x40006040UL
#define EP0_RX_ADDR        0x40006080UL
#define EP1_TX_ADDR        0x400060C0UL
#define EP1_RX_ADDR        0x40006100UL

/* 512-byte buffers aligned */
static u8 ep0_tx_buf[64]  __attribute__((aligned(4)));
static u8 ep0_rx_buf[64]  __attribute__((aligned(4)));
static u8 ep1_tx_buf[64]  __attribute__((aligned(4)));
static u8 ep1_rx_buf[64]  __attribute__((aligned(4)));

typedef struct {
    volatile u32 addr_tx;
    volatile u32 count_tx;
    volatile u32 addr_rx;
    volatile u32 count_rx;
} usb_desc_t;

static usb_desc_t *ep0_desc = (usb_desc_t *)EP0_DESC;
static usb_desc_t *ep1_desc = (usb_desc_t *)EP1_DESC;

static u8  usb_state        = 0;
static u8  usb_addr         = 0;
static u8  usb_config_value = 0;
static u8  usb_ep0_phase    = 0;
static u16 usb_setup_len    = 0;
static u16 usb_setup_idx    = 0;

static void usb_write_pma(u16 addr, const u8 *src, u16 len) {
    volatile u16 *pma = (volatile u16 *)(0x40006000UL + addr);
    for (u16 i = 0; i < len; i += 2) {
        u16 val = (i + 1 < len) ? (src[i + 1] << 8) | src[i] : src[i];
        pma[i >> 1] = val;
    }
}

static void usb_read_pma(u16 addr, u8 *dst, u16 len) {
    volatile u16 *pma = (volatile u16 *)(0x40006000UL + addr);
    for (u16 i = 0; i < len; i += 2) {
        u16 val = pma[i >> 1];
        dst[i] = val & 0xFF;
        if (i + 1 < len) dst[i + 1] = (val >> 8) & 0xFF;
    }
}

static void usb_reset(void) {
    USB_CNTR = (1<<0);        // FRES
    USB_CNTR = 0;             // clear FRES
    USB_DADDR = 0;            // address 0
    ep0_desc->addr_tx = 0x40;
    ep0_desc->count_tx = 0;
    ep0_desc->addr_rx = 0x80;
    ep0_desc->count_rx = 0x8400; // BL_SIZE=1, NUM_BLOCK=2
    ep1_desc->addr_tx = 0xC0;
    ep1_desc->count_tx = 0;
    ep1_desc->addr_rx = 0x100;
    ep1_desc->count_rx = 0x8400;
}

static void usb_set_addr(u8 addr) {
    USB_DADDR = (addr & 0x7F) | (1<<7);
}

static u8 usb_control_send(const u8 *data, u16 len) {
    u16 chunk = (len > 64) ? 64 : len;
    usb_write_pma(0x40, data, chunk);
    ep0_desc->count_tx = chunk;
    ep0_desc->addr_tx = 0x40;
    return 0;
}

static u8 usb_control_recv(u8 *data, u16 len) {
    u16 chunk = (len > 64) ? 64 : len;
    usb_read_pma(0x80, data, chunk);
    ep0_desc->count_rx = 0x8400;
    return 0;
}

static void usb_setup_handler(void) {
    u8 setup[8];
    usb_read_pma(0x80, setup, 8);
    u16 wLength = setup[6] | (setup[7] << 8);
    switch (setup[1]) {
        case 0x06: /* GET_DESCRIPTOR */
            if (setup[3] == 0x01) { /* device */
                static const u8 dev_desc[18] = {
                    0x12, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 64,
                    0x34, 0x12, 0x78, 0x56, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01
                };
                usb_control_send(dev_desc, 18);
            }
            break;
        case 0x05: /* SET_ADDRESS */
            usb_addr = setup[2];
            usb_set_addr(0); // stage 0
            break;
    }
}

static void usb_poll(void) {
    u32 istr = USB_ISTR;
    if (istr & (1<<15)) usb_reset();
    if (istr & (1<<0))  usb_setup_handler();
}

void usb_init(void) {
    usb_reset();
    USB_CNTR = (1<<12) | (1<<0); // CTRM + RESETM
}

