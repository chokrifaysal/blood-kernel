/*
 * canfd.c – PIC32MZ CAN-FD @ 1 Mbit/s nominal, 4 Mbit/s data
 */

#include "canfd.h"
#include "kernel/types.h"

#define C1_BASE 0xBF88B000UL

typedef volatile struct {
    u32 CON;
    u32 CFG;
    u32 TXQCON;
    u32 FIFOCON[32];
    u32 FIFOUA[32];
} CAN_TypeDef;

static CAN_TypeDef* const C1 = (CAN_TypeDef*)C1_BASE;

void canfd_init(u32 bitrate) {
    /* 50 MHz / (bitrate * 2) – 1 = 24 */
    C1->CFG = 24;
    C1->CON = (1<<7) | (1<<15); // ON + CANEN
}

u8 canfd_send(const canfd_frame_t* f) {
    /* TX FIFO 0 */
    u32* tx = (u32*)C1->FIFOUA[0];
    tx[0] = (f->id << 18) | (f->len << 16);
    for (u8 i = 0; i < f->len; i += 4) {
        tx[1 + i/4] = *(u32*)(f->data + i);
    }
    C1->FIFOCON[0] |= (1<<0); // TXREQ
    return 0;
}

u8 canfd_recv(canfd_frame_t* f) {
    /* RX FIFO 1 */
    u32* rx = (u32*)C1->FIFOUA[1];
    f->id  = (rx[0] >> 18) & 0x1FFFFFFF;
    f->len = (rx[0] >> 16) & 0x0F;
    for (u8 i = 0; i < f->len; i += 4) {
        *(u32*)(f->data + i) = rx[1 + i/4];
    }
    C1->FIFOCON[1] |= (1<<1); // RXACK
    return 0;
}
