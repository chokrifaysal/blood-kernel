/*
 * can.c – CAN1, 500 kbit/s, 16 TX + 16 RX FIFO
 */

#include "kernel/types.h"

#define CAN1_BASE 0x40006400UL
#define CAN_CTL  (*(volatile u32*)(CAN1_BASE + 0x00))
#define CAN_BT   (*(volatile u32*)(CAN1_BASE + 0x18))
#define CAN_TMI0 (*(volatile u32*)(CAN1_BASE + 0x180))
#define CAN_TMP0 (*(volatile u32*)(CAN1_BASE + 0x184))
#define CAN_TMD0 (*(volatile u32*)(CAN1_BASE + 0x188))
#define CAN_RFIFO0 (*(volatile u32*)(CAN1_BASE + 0x1B0))

void can_init(void) {
    CAN_CTL |= (1<<0); // reset
    /* 108 MHz / (8 * (1+7+8)) = 500 kbit/s */
    CAN_BT = (7<<16) | (8<<20) | (1<<24);
    CAN_CTL &= ~(1<<0);
}

void can_send(u32 id, const u8* data, u8 len) {
    CAN_TMI0 = id << 21;
    CAN_TMP0 = len << 0;
    *(volatile u32*)(&CAN_TMD0) = *(u32*)data;
    CAN_CTL |= (1<<2); // TXREQ
}

u8 can_recv(u32* id, u8* buf) {
    if (!(CAN_RFIFO0 & (1<<3))) return 0;
    *id = (CAN_RFIFO0 >> 21) & 0x7FF;
    *(u32*)buf = *(volatile u32*)(CAN1_BASE + 0x1B4);
    return 8;
}
