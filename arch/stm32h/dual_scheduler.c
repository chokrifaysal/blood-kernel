/*
 * dual_scheduler.c – separate schedulers for M7 & M4
 */

#include "kernel/sched.h"
#include "kernel/types.h"
#include "arch/stm32h745/ipc_mailbox_500loc.c"

static struct task task_pool_m7[32];
static struct task task_pool_m4[16];

void sched_init_m7(void) {
    for (int i = 0; i < 32; ++i) task_pool_m7[i].state = 0;
}

void sched_start_m7(void) {
    struct task *t = &task_pool_m7[0];
    t->state = 1;
    context_switch(0, t->sp);
}

void sched_init_m4(void) {
    for (int i = 0; i < 16; ++i) task_pool_m4[i].state = 0;
}

void sched_start_m4(void) {
    struct task *t = &task_pool_m4[0];
    t->state = 1;
    context_switch(0, t->sp);
}
