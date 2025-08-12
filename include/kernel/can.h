/*
 * can.h - automotive CAN driver
 */

#ifndef _BLOOD_CAN_H
#define _BLOOD_CAN_H

#include "kernel/types.h"

#define CAN_STD_ID 11
#define CAN_EXT_ID 29

typedef struct {
    u32 id;
    u8  len;
    u8  data[8];
} can_frame_t;

typedef enum {
    CAN_OK = 0,
    CAN_ERR_TX,
    CAN_ERR_RX,
    CAN_ERR_BUSOFF
} can_err_t;

void can_init(u32 baud);
can_err_t can_send(const can_frame_t* frame);
can_err_t can_recv(can_frame_t* frame);
void can_set_filter(u32 id, u32 mask);
u8 can_tx_mailbox_free(void);
void can_irq_handler(void);

#endif
