/*
 * canfd.h – STM32H745 FDCAN driver
 */

#ifndef CANFD_H
#define CANFD_H

#include "kernel/types.h"

typedef struct {
    u32 id;
    u8 len;
    u8 flags;
    u8 data[64];
} canfd_frame_t;

/* CAN-FD flags */
#define CANFD_FDF (1<<0)  // FD format
#define CANFD_BRS (1<<1)  // Bit rate switch

/* CAN states */
#define CAN_STATE_ACTIVE    0
#define CAN_STATE_PASSIVE   1
#define CAN_STATE_BUS_OFF   2

void canfd_init(u32 bitrate);
u8 canfd_send(const canfd_frame_t* f);
u8 canfd_recv(canfd_frame_t* f);
void canfd_set_filter(u16 id, u16 mask);
u8 canfd_get_state(void);

#endif
