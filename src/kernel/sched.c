/*
 * sched.c - round-robin scheduler with timer preemption
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
            
#ifdef __arm__
            // ARM: stack grows down from RAM top
            task_pool[i].stack_base = 0x2001C000 - (i * (stack_size + 64));
#else
            // x86: give each task 1k stack in low mem
            task_pool[i].stack_base = 0x8000 + (i * 0x400);
#endif
            
            task_pool[i].sp = (u32*)(task_pool[i].stack_base + stack_size - 32);
            
            // initialize stack frame
            u32* sp = task_pool[i].sp;
#ifdef __arm__
            sp[0] = 0x01000000;      // xPSR
            sp[1] = (u32)entry;      // PC
            sp[2] = 0xFFFFFFFD;      // LR
#endif
            
#ifdef __x86_64__
            // x86 stack frame
            sp[0] = (u32)entry;      // return address
#endif
            
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
    u32 start = (current_task) ? (current_task - task_pool) : 0;
    
    for (u32 i = 1; i <= MAX_TASKS; i++) {
        u32 idx = (start + i) % MAX_TASKS;
        if (task_pool[idx].state == 0 && task_pool[idx].pid != 0) {
            next = &task_pool[idx];
            break;
        }
    }
    
    if (next != current_task) {
        struct task* old = current_task;
        current_task = next;
        if (old) old->state = 0;
        next->state = 1;
        context_switch(&old->sp, next->sp);
    }
}

void task_exit(void) {
    current_task->state = 0;
    task_yield();   // never returns
}
