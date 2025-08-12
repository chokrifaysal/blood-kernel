/*
 * sched.c - round-robin scheduler with canaries and spinlocks
 */

#include "kernel/sched.h"
#include "kernel/types.h"
#include "kernel/spinlock.h"
#include "common/compiler.h"
#include "uart.h"

static struct task task_pool[MAX_TASKS];
static struct task* current_task = 0;
static u32 next_pid = 1;
static spinlock_t sched_lock = {0};

static void write_canary(struct task* t) {
    u32* canary_ptr = (u32*)(t->stack_base);
    *canary_ptr = STACK_CANARY;
    t->canary = STACK_CANARY;
}

static u8 check_canary(struct task* t) {
    u32* canary_ptr = (u32*)(t->stack_base);
    return (*canary_ptr == t->canary) ? 1 : 0;
}

void sched_init(void) {
    for (int i = 0; i < MAX_TASKS; i++) {
        task_pool[i].state = 0;
        task_pool[i].pid = 0;
    }
    uart_puts("Scheduler initialized\r\n");
}

u32 task_create(task_entry_t entry, void* arg, u32 stack_size) {
    spin_lock(&sched_lock);
    
    for (int i = 0; i < MAX_TASKS; i++) {
        if (task_pool[i].state == 0) {
            task_pool[i].pid = next_pid++;
            task_pool[i].stack_size = stack_size;
            
#ifdef __arm__
            task_pool[i].stack_base = 0x2001C000 - (i * (stack_size + 64));
#else
            task_pool[i].stack_base = 0x8000 + (i * 0x400);
#endif
            
            task_pool[i].sp = (u32*)(task_pool[i].stack_base + stack_size - 32);
            write_canary(&task_pool[i]);
            
            u32* sp = task_pool[i].sp;
#ifdef __arm__
            sp[0] = 0x01000000;
            sp[1] = (u32)entry;
            sp[2] = 0xFFFFFFFD;
#endif
            
#ifdef __x86_64__
            sp[0] = (u32)entry;
#endif
            
            task_pool[i].state = 0;
            spin_unlock(&sched_lock);
            return task_pool[i].pid;
        }
    }
    
    spin_unlock(&sched_lock);
    return 0;
}

void task_stack_check(void) {
    for (int i = 0; i < MAX_TASKS; i++) {
        if (task_pool[i].pid != 0 && !check_canary(&task_pool[i])) {
            uart_puts("Stack overflow task ");
            uart_hex(task_pool[i].pid);
            uart_puts("\r\n");
            task_pool[i].state = 0;   // kill it
        }
    }
}

void sched_start(void) {
    if (!current_task) {
        for (int i = 0; i < MAX_TASKS; i++) {
            if (task_pool[i].state == 0 && task_pool[i].pid != 0) {
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
    spin_lock(&sched_lock);
    
    task_stack_check();
    
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
        spin_unlock(&sched_lock);
        context_switch(&old->sp, next->sp);
    } else {
        spin_unlock(&sched_lock);
    }
}

void task_exit(void) {
    spin_lock(&sched_lock);
    current_task->state = 0;
    spin_unlock(&sched_lock);
    task_yield();
}
