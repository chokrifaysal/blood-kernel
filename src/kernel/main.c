/*
 * kernel_main 
 * Both x86 and ARM entry point
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

static msg_queue_t uart_rx_q;

void kernel_main(u32 magic, u32 addr) {
    uart_early_init();
    
    uart_puts("\r\n");
    uart_puts("BLOOD_KERNEL v0.6 - booted\r\n");
    
    if (magic == 0x2BADB002) {
        detect_memory_x86(addr);
    } else {
        detect_memory_arm();
    }
    
    sched_init();
    timer_init();
    gpio_init();
    msg_init(&uart_rx_q);
    can_init(500000);   // 500 kbit/s
    
    task_create(idle_task, 0, 256);
    task_create(test_task, 0, 256);
    task_create(blink_task, 0, 256);
    task_create(can_task, 0, 512);
    
    uart_puts("Starting scheduler...\r\n");
    sched_start();
    
    while (1) {
        __asm__ volatile("hlt");
    }
}

void idle_task(void) {
    while (1) {
        __asm__ volatile("nop");
    }
}

void test_task(void) {
    static spinlock_t print_lock = {0};
    u32 cnt = 0;
    
    while (1) {
        spin_lock(&print_lock);
        uart_puts("tick ");
        uart_hex(cnt++);
        uart_puts("\r\n");
        spin_unlock(&print_lock);
        task_yield();
    }
}

void blink_task(void) {
    gpio_set_mode(PA5, GPIO_OUT);
    
    while (1) {
        gpio_toggle(PA5);
        timer_delay(200);
    }
}

void can_task(void) {
    static can_frame_t tx_frame = {
        .id = 0x123,
        .len = 8,
        .data = {0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE}
    };
    
    while (1) {
        can_send(&tx_frame);
        timer_delay(1000);
        tx_frame.data[0]++;
    }
}
