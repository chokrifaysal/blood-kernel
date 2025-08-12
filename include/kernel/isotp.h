/*
 * isotp.h - ISO-TP transport (CAN-TP) for diagnostics
 */

#ifndef _BLOOD_ISOTP_H
#define _BLOOD_ISOTP_H

#include "kernel/types.h"

#define ISOTP_MAX_PAYLOAD 4096
#define ISOTP_ST_MIN      0x14   // 20 ms separation time

typedef struct {
    u32 id;
    u8  data[ISOTP_MAX_PAYLOAD];
    u16 len;
} isotp_msg_t;

void isotp_init(void);
u8   isotp_send(const isotp_msg_t* msg);
u8   isotp_recv(isotp_msg_t* msg);

#endif
