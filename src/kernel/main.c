/*
 * kernel_main 
 * Both x86 and ARM entry point
 */

#include "kernel/types.h"
#include "kernel/uart.h"
#include "kernel/mem.h"
#include "kernel/sched.h"

void kernel_main(u32 magic, u32 addr) {
    uart_early_init();          // fire up UART0
    
    uart_puts("\r\n");
    uart_puts("BLOOD_KERNEL v0.3 - booted\r\n");
    
    // detect and report RAM
    if (magic == 0x2BADB002) {
        detect_memory_x86(addr);    // addr = multiboot info
    } else {
        detect_memory_arm();        // bare-metal ARM
    }
    
    sched_init();               // bring up scheduler
    
    // create idle task
    task_create(idle_task, 0, 256);
    
    // create test task
    task_create(test_task, 0, 256);
    
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
    while (1) {
        uart_puts("task A\r\n");
        for (volatile int i = 0; i < 1000000; i++);
    }
}
