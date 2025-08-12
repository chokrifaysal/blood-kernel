/*
 * sched.h - minimal round-robin scheduler
 */

#ifndef _BLOOD_SCHED_H
#define _BLOOD_SCHED_H

#include "kernel/types.h"

#define MAX_TASKS 32
#define KERNEL_STACK_SIZE 512

typedef void (*task_entry_t)(void);

struct task {
    u32* sp;                    // current stack pointer
    u32 stack_base;             // bottom of stack
    u32 stack_size;
    u8  state;                  // 0=ready, 1=running, 2=blocked
    u8  priority;
    u32 pid;
};

void sched_init(void);
void sched_start(void);
u32 task_create(task_entry_t entry, void* arg, u32 stack_size);
void task_yield(void);
void task_exit(void);

#endif
