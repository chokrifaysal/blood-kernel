/*
 * sched.c - round-robin scheduler, static TCB pool
 */

#include "kernel/sched.h"
#include "kernel/types.h"
#include "common/compiler.h"
#include "uart.h"

static struct task task_pool[MAX_TASKS];
static struct task* current_task = 0;
static u32 next_pid = 1;
static u8 sched_ready = 0;

void sched_init(void) {
    for (int i = 0; i < MAX_TASKS; i++) {
        task_pool[i].state = 0;
        task_pool[i].pid = 0;
    }
    uart_puts("Scheduler initialized\r\n");
}

u32 task_create(task_entry_t entry, void* arg, u32 stack_size) {
    for (int i = 0; i < MAX_TASKS; i++) {
        if (task_pool[i].state == 0) {
            task_pool[i].pid = next_pid++;
            task_pool[i].stack_size = stack_size;
            task_pool[i].stack_base = 0x2001C000 - (i * (stack_size + 64));  // rough placement
            task_pool[i].sp = (u32*)(task_pool[i].stack_base + stack_size - 32);
            
            // initialize stack frame
            u32* sp = task_pool[i].sp;
            sp[0] = 0x01000000;      // xPSR (thumb bit)
            sp[1] = (u32)entry;      // PC
            sp[2] = 0xFFFFFFFD;      // LR (exit)
            sp[3] = 0;               // R12
            sp[4] = 0;               // R3
            sp[5] = 0;               // R2
            sp[6] = 0;               // R1
            sp[7] = (u32)arg;        // R0
            
            task_pool[i].state = 0;   // ready
            return task_pool[i].pid;
        }
    }
    return 0;   // fail
}

void sched_start(void) {
    if (!current_task) {
        for (int i = 0; i < MAX_TASKS; i++) {
            if (task_pool[i].state == 0) {
                current_task = &task_pool[i];
                current_task->state = 1;
                break;
            }
        }
    }
    
    if (current_task) {
        uart_puts("Switching to first task\r\n");
        context_switch(0, current_task->sp);
    }
}

void task_yield(void) {
    // find next ready task
    struct task* next = current_task;
    do {
        next++;
        if (next >= &task_pool[MAX_TASKS])
            next = &task_pool[0];
    } while (next->state != 0 && next != current_task);
    
    if (next != current_task) {
        struct task* old = current_task;
        current_task = next;
        old->state = 0;
        next->state = 1;
        context_switch(&old->sp, next->sp);
    }
}

void task_exit(void) {
    current_task->state = 0;
    task_yield();   // never returns
}
