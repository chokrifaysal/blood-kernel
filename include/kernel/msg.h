/*
 * msg.h - tiny fixed-length message queues
 */

#ifndef _BLOOD_MSG_H
#define _BLOOD_MSG_H

#include "kernel/types.h"

#define MSG_SIZE 16          // bytes per message
#define MSG_DEPTH 8          // messages per queue

typedef struct {
    u8 data[MSG_SIZE];
} msg_t;

typedef struct {
    msg_t buf[MSG_DEPTH];
    u8 head;
    u8 tail;
    u8 count;
    spinlock_t lock;
} msg_queue_t;

void msg_init(msg_queue_t* q);
u8 msg_send(msg_queue_t* q, const msg_t* msg);
u8 msg_recv(msg_queue_t* q, msg_t* msg);

#endif
