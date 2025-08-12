/*
 * kernel_main – v1.1 with safety tests
 */

#include "kernel/types.h"
#include "kernel/uart.h"
#include "kernel/sched.h"
#include "kernel/timer.h"
#include "kernel/can.h"
#include "kernel/wdog.h"
#include "kernel/kprintf.h"
#include "kernel/isotp.h"
#include "kernel/flash.h"

extern void safety_test_task(void);

void kernel_main(u32 magic, u32 addr) {
    kprintf("BLOOD_KERNEL v1.1 safety release\r\n");
    
    sched_init();
    timer_init();
    can_init(500000);
    wdog_init(2000);
    isotp_init();
    
    task_create(idle_task, 0, 256);
    task_create(safety_test_task, 0, 512);
    
    sched_start();
}
