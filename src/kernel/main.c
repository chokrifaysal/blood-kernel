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

void kernel_main(u32 magic, u32 addr) {
    uart_early_init();          // fire up UART0
    
    uart_puts("\r\n");
    uart_puts("BLOOD_KERNEL v0.5 - booted\r\n");
    
    // detect and report RAM
    if (magic == 0x2BADB002) {
        detect_memory_x86(addr);    // addr = multiboot info
    } else {
        detect_memory_arm();        // bare-metal ARM
    }
    
    sched_init();               // bring up scheduler
    timer_init();               // start 1 ms ticks
    gpio_init();                // init GPIO ports
    
    // create tasks
    task_create(idle_task, 0, 256);
    task_create(test_task, 0, 256);
    task_create(blink_task, 0, 256);
    
    uart_puts("Starting scheduler...\r\n");
    sched_start();              // never returns
    
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
        uart_puts("task A: ");
        uart_hex(cnt++);
        uart_puts("\r\n");
        spin_unlock(&print_lock);
        task_yield();
    }
}

void blink_task(void) {
    gpio_set_mode(PA5, GPIO_OUT);   // LED pin
    
    while (1) {
        gpio_toggle(PA5);
        timer_delay(500);           // 500 ms
    }
}
