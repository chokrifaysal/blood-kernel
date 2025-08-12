/*
 * canfd.h – PIC32MZ CAN-FD controller
 */

#ifndef _BLOOD_CANFD_H
#define _BLOOD_CANFD_H

#include "kernel/types.h"

#define CANFD_MAX_PAYLOAD 64

typedef struct {
    u32 id;
    u8  len;
    u8  data[CANFD_MAX_PAYLOAD];
} canfd_frame_t;

void canfd_init(u32 bitrate);
u8   canfd_send(const canfd_frame_t* f);
u8   canfd_recv(canfd_frame_t* f);

#endif
