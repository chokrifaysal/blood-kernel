/*
 * can.c - CAN driver skeleton for STM32 bxCAN
 */

#include "kernel/can.h"
#include "kernel/types.h"
#include "kernel/msg.h"
#include "kernel/spinlock.h"

#ifdef __arm__

#define CAN1_BASE 0x40006400
#define CAN2_BASE 0x40006800
#define RCC_APB1ENR (*(volatile u32*)0x40023840)

typedef struct {
    volatile u32 TIR;
    volatile u32 TDTR;
    volatile u32 TDLR;
    volatile u32 TDHR;
} CAN_TxMailBox_TypeDef;

typedef struct {
    volatile u32 RIR;
    volatile u32 RDTR;
    volatile u32 RDLR;
    volatile u32 RDHR;
} CAN_FIFOMailBox_TypeDef;

static CAN_TxMailBox_TypeDef* const CAN1_TX = (CAN_TxMailBox_TypeDef*)(CAN1_BASE + 0x180);
static CAN_FIFOMailBox_TypeDef* const CAN1_RX = (CAN_FIFOMailBox_TypeDef*)(CAN1_BASE + 0x1B0);

static msg_queue_t can_rx_q;
static spinlock_t can_lock = {0};

void can_init(u32 baud) {
    (void)baud;   // TODO: set baudrate
    
    RCC_APB1ENR |= (1<<25);   // CAN1 clock
    
    msg_init(&can_rx_q);
    uart_puts("CAN initialized (stub)\r\n");
}

can_err_t can_send(const can_frame_t* frame) {
    spin_lock(&can_lock);
    
    // TODO: real CAN TX logic
    uart_puts("CAN send: id=");
    uart_hex(frame->id);
    uart_puts(" len=");
    uart_hex(frame->len);
    uart_puts("\r\n");
    
    spin_unlock(&can_lock);
    return CAN_OK;
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
    (void)id;
    (void)mask;
    // TODO: configure hardware filters
}

#else
// x86 PC has no CAN - stubs
void can_init(u32 baud) { (void)baud; }
can_err_t can_send(const can_frame_t* frame) { (void)frame; return CAN_OK; }
can_err_t can_recv(can_frame_t* frame) { (void)frame; return CAN_ERR_RX; }
void can_set_filter(u32 id, u32 mask) { (void)id; (void)mask; }

#endif
