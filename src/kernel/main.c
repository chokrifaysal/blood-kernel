/*
 * kernel_main - full automotive stack
 */

#include "kernel/types.h"
#include "kernel/uart.h"
#include "kernel/mem.h"
#include "kernel/sched.h"
#include "kernel/timer.h"
#include "kernel/gpio.h"
#include "kernel/spinlock.h"
#include "kernel/msg.h"
#include "kernel/can.h"
#include "kernel/mpu.h"
#include "kernel/elf.h"
#include "kernel/wdog.h"
#include "kernel/kprintf.h"
#include "kernel/sd.h"
#include "kernel/log.h"

void kernel_main(u32 magic, u32 addr) {
    uart_early_init();
    kprintf("\r\nBLOOD_KERNEL v0.9 booted\r\n");
    
    if (magic == 0x2BADB002) {
        detect_memory_x86(addr);
    } else {
        detect_memory_arm();
    }
    
    sched_init();
    timer_init();
    gpio_init();
    msg_init(&uart_rx_q);
    can_init(500000);
    wdog_init(2000);
    log_init();
    
#ifdef __arm__
    mpu_init();
#endif
    
    task_create(idle_task, 0, 256);
    task_create(blink_task, 0, 256);
    task_create(can_task, 0, 512);
    task_create(logger_task, 0, 512);
    
    kprintf("All drivers up\r\n");
    sched_start();
}

void idle_task(void) {
    while (1) {
        wdog_refresh();
    }
}

void blink_task(void) {
    gpio_set_mode(PA5, GPIO_OUT);
    while (1) {
        gpio_toggle(PA5);
        timer_delay(500);
    }
}

void can_task(void) {
    can_frame_t tx = {.id = 0x123, .len = 8, .data = {0xAA}};
    while (1) {
        tx.data[0]++;
        can_send(&tx);
        timer_delay(1000);
    }
}

void logger_task(void) {
    can_frame_t rx;
    while (1) {
        if (can_recv(&rx) == 0) {
            log_can(&rx);
        }
        timer_delay(100);
    }
}
