/*
 * kernel_main 
 * Both x86 and ARM entry point
 */

#include "kernel/types.h"
#include "kernel/uart.h"
#include "kernel/mem.h"
#include "common/compiler.h"

void kernel_main(u32 magic, u32 addr) {
    uart_early_init();          // fire up UART0
    
    uart_puts("\r\n");
    uart_puts("BLOOD_KERNEL v0.2 - booted\r\n");
    
    // detect and report RAM
    if (magic == 0x2BADB002) {
        detect_memory_x86(addr);    // addr = multiboot info
    } else {
        detect_memory_arm();        // bare-metal ARM
    }
    
    uart_puts("System ready.\r\n");
    
    while (1) {
        __asm__ volatile("hlt");
    }
}
