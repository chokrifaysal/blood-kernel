/*
 * can.c - STM32 bxCAN driver with RX FIFO
 */

#include "kernel/can.h"
#include "kernel/types.h"
#include "kernel/msg.h"
#include "kernel/spinlock.h"
#include "uart.h"

#ifdef __arm__

#define CAN1_BASE 0x40006400
#define RCC_APB1ENR (*(volatile u32*)0x40023840)
#define NVIC_ISER1  (*(volatile u32*)0xE000E104)

typedef volatile struct {
    u32 TIR;    u32 TDTR;   u32 TDLR;   u32 TDHR;
} CAN_TxMailBox_TypeDef;

typedef volatile struct {
    u32 RIR;    u32 RDTR;   u32 RDLR;   u32 RDHR;
} CAN_FIFOMailBox_TypeDef;

typedef volatile struct {
    u32 FR1;    u32 FR2;
} CAN_Filter_TypeDef;

#define CAN1_TX   ((CAN_TxMailBox_TypeDef*)(CAN1_BASE + 0x180))
#define CAN1_RX0  ((CAN_FIFOMailBox_TypeDef*)(CAN1_BASE + 0x1B0))
#define CAN1_RX1  ((CAN_FIFOMailBox_TypeDef*)(CAN1_BASE + 0x1C0))
#define CAN1_FMR  (*(volatile u32*)(CAN1_BASE + 0x200))
#define CAN1_FM1R (*(volatile u32*)(CAN1_BASE + 0x204))
#define CAN1_FS1R (*(volatile u32*)(CAN1_BASE + 0x20C))
#define CAN1_FFA1R (*(volatile u32*)(CAN1_BASE + 0x214))
#define CAN1_FA1R (*(volatile u32*)(CAN1_BASE + 0x21C))

static msg_queue_t can_rx_q;
static spinlock_t can_lock = {0};

void can_init(u32 baud) {
    // enable clocks
    RCC_APB1ENR |= (1<<25);   // CAN1
    RCC_APB1ENR |= (1<<14);   // GPIOA alt-fn
    
    // enter init mode
    *(volatile u32*)(CAN1_BASE + 0x00) |= (1<<0);
    while (!(*(volatile u32*)(CAN1_BASE + 0x00) & (1<<0)));
    
    // set baudrate 500k @ 42MHz APB1
    *(volatile u32*)(CAN1_BASE + 0x18) = (6<<24)|(5<<20)|(3<<16)|5;
    
    // leave init
    *(volatile u32*)(CAN1_BASE + 0x00) &= ~(1<<0);
    while (*(volatile u32*)(CAN1_BASE + 0x00) & (1<<0));
    
    // filters: accept all
    CAN1_FMR |= (1<<0);   // init filters
    CAN1_FA1R = 0;        // disable all
    CAN1_FMR &= ~(1<<0);  // activate
    
    // enable RX0 IRQ
    NVIC_ISER1 |= (1<<21);
    
    msg_init(&can_rx_q);
    uart_puts("CAN ready 500k\r\n");
}

can_err_t can_send(const can_frame_t* frame) {
    u32 mb = 0;
    
    spin_lock(&can_lock);
    if (!(CAN1_TX[mb].TIR & (1<<0))) {   // TXE
        CAN1_TX[mb].TIR = (frame->id << 21) | (1<<1);  // STD ID
        CAN1_TX[mb].TDTR = frame->len;
        CAN1_TX[mb].TDLR = *(u32*)frame->data;
        CAN1_TX[mb].TDHR = *(u32*)(frame->data + 4);
        CAN1_TX[mb].TIR |= (1<<0);   // request
        spin_unlock(&can_lock);
        return CAN_OK;
    }
    spin_unlock(&can_lock);
    return CAN_ERR_TX;
}

can_err_t can_recv(can_frame_t* frame) {
    msg_t msg;
    if (msg_recv(&can_rx_q, &msg)) {
        *frame = *(can_frame_t*)msg.data;
        return CAN_OK;
    }
    return CAN_ERR_RX;
}

void can_set_filter(u32 id, u32 mask) {
    // TODO: configure hardware filter bank
    (void)id; (void)mask;
}

u8 can_tx_mailbox_free(void) {
    return !(CAN1_TX[0].TIR & (1<<0));
}

// RX0 IRQ
void CEC_CAN_IRQHandler(void) {
    if (*(volatile u32*)(CAN1_BASE + 0x08) & (1<<0)) {   // FMP0
        can_frame_t frm;
        frm.id  = (CAN1_RX0->RIR >> 21) & 0x7FF;
        frm.len = CAN1_RX0->RDTR & 0x0F;
        *(u32*)frm.data     = CAN1_RX0->RDLR;
        *(u32*)(frm.data+4) = CAN1_RX0->RDHR;
        
        msg_t msg;
        *(can_frame_t*)msg.data = frm;
        msg_send(&can_rx_q, &msg);
        
        // release FIFO
        *(volatile u32*)(CAN1_BASE + 0x08) |= (1<<5);
    }
}

#else
// x86 PC has no CAN - stubs
void can_init(u32 baud) { (void)baud; }
can_err_t can_send(const can_frame_t* frame) { (void)frame; return CAN_OK; }
can_err_t can_recv(can_frame_t* frame) { (void)frame; return CAN_ERR_RX; }
void can_set_filter(u32 id, u32 mask) { (void)id; (void)mask; }
u8 can_tx_mailbox_free(void) { return 1; }
void can_irq_handler(void) {}

#endif
