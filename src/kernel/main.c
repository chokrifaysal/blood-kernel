/*
 * kernel_main 
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

void kernel_main(u32 magic, u32 addr) {
    uart_early_init();
    kprintf("\r\nBLOOD_KERNEL v0.8 booted\r\n");
    
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
    wdog_init(2000);        // 2 sec watchdog
    
#ifdef __arm__
    mpu_init();
#endif
    
    kprintf("Starting watchdog...\r\n");
    wdog_refresh();
    
    task_create(idle_task, 0, 256);
    task_create(blink_task, 0, 256);
    task_create(can_task, 0, 512);
    
    kprintf("Starting scheduler...\r\n");
    sched_start();
    
    while (1) {
        __asm__ volatile("hlt");
    }
}

void idle_task(void) {
    while (1) {
        wdog_refresh();
        __asm__ volatile("nop");
    }
}

void blink_task(void) {
    gpio_set_mode(PA5, GPIO_OUT);
    u32 cnt = 0;
    
    while (1) {
        gpio_toggle(PA5);
        kprintf("blink %d\r\n", cnt++);
        timer_delay(200);
    }
}

void can_task(void) {
    can_frame_t tx = {.id = 0x123, .len = 8, .data = {0xAA}};
    can_frame_t rx;
    
    while (1) {
        tx.data[0]++;
        can_send(&tx);
        
        if (can_recv(&rx) == CAN_OK) {
            kprintf("CAN RX id=%x len=%d data=%02x\r\n",
                   rx.id, rx.len, rx.data[0]);
        }
        timer_delay(1000);
    }
}
