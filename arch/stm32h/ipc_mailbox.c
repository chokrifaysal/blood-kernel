/*
 * ipc_mailbox.c – dual-core IPC mailbox
 * CM7 ↔ CM4, lock-free ring buffer, IRQ trigger
 */

#include "kernel/types.h"
#include "kernel/spinlock.h"

#define MAILBOX_BASE   0x38001000UL
#define MAILBOX_SIZE   256
#define MAILBOX_IRQ_M7  55
#define MAILBOX_IRQ_M4  56

typedef struct {
    u32 head;
    u32 tail;
    u8  data[MAILBOX_SIZE];
} mailbox_t;

static mailbox_t *mbox_m7 = (mailbox_t *)(MAILBOX_BASE + 0x000);
static mailbox_t *mbox_m4 = (mailbox_t *)(MAILBOX_BASE + 0x400);
static spinlock_t m7_lock = {0};
static spinlock_t m4_lock = {0};

static inline u32 mbox_free(mailbox_t *m) {
    return (m->tail - m->head - 1) & (MAILBOX_SIZE - 1);
}

static inline u32 mbox_used(mailbox_t *m) {
    return (m->head - m->tail) & (MAILBOX_SIZE - 1);
}

static void mbox_send(mailbox_t *m, const u8 *buf, u32 len) {
    spin_lock(&m7_lock);
    for (u32 i = 0; i < len; ++i) {
        u32 idx = (m->head + i) & (MAILBOX_SIZE - 1);
        m->data[idx] = buf[i];
    }
    m->head = (m->head + len) & (MAILBOX_SIZE - 1);
    spin_unlock(&m7_lock);
}

static u32 mbox_recv(mailbox_t *m, u8 *buf, u32 max) {
    spin_lock(&m4_lock);
    u32 used = mbox_used(m);
    u32 copy = used > max ? max : used;
    for (u32 i = 0; i < copy; ++i) {
        u32 idx = (m->tail + i) & (MAILBOX_SIZE - 1);
        buf[i] = m->data[idx];
    }
    m->tail = (m->tail + copy) & (MAILBOX_SIZE - 1);
    spin_unlock(&m4_lock);
    return copy;
}

void ipc_init(void) {
    NVIC->ISER[1] |= (1<<(MAILBOX_IRQ_M7 - 32));
    NVIC->ISER[1] |= (1<<(MAILBOX_IRQ_M4 - 32));
}

void ipc_send_m7(const u8 *buf, u32 len) {
    mbox_send(mbox_m7, buf, len);
    *(volatile u32*)0xE000E100 = (1<<MAILBOX_IRQ_M4);
}

void ipc_send_m4(const u8 *buf, u32 len) {
    mbox_send(mbox_m4, buf, len);
    *(volatile u32*)0xE000E100 = (1<<MAILBOX_IRQ_M7);
}

u32 ipc_recv_m7(u8 *buf, u32 max) {
    return mbox_recv(mbox_m7, buf, max);
}

u32 ipc_recv_m4(u8 *buf, u32 max) {
    return mbox_recv(mbox_m4, buf, max);
}
