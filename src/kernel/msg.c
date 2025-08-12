/*
 * msg.c - lock-free ring buffer for small messages
 */

#include "kernel/msg.h"
#include "kernel/spinlock.h"

void msg_init(msg_queue_t* q) {
    q->head = q->tail = q->count = 0;
    q->lock = (spinlock_t){0};
}

u8 msg_send(msg_queue_t* q, const msg_t* msg) {
    spin_lock(&q->lock);
    
    if (q->count >= MSG_DEPTH) {
        spin_unlock(&q->lock);
        return 0;   // full
    }
    
    q->buf[q->head] = *msg;
    q->head = (q->head + 1) % MSG_DEPTH;
    q->count++;
    
    spin_unlock(&q->lock);
    return 1;
}

u8 msg_recv(msg_queue_t* q, msg_t* msg) {
    spin_lock(&q->lock);
    
    if (q->count == 0) {
        spin_unlock(&q->lock);
        return 0;   // empty
    }
    
    *msg = q->buf[q->tail];
    q->tail = (q->tail + 1) % MSG_DEPTH;
    q->count--;
    
    spin_unlock(&q->lock);
    return 1;
}
