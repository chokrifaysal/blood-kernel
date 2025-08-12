/*
 * kernel_main 
 * Both x86 and ARM entry point
 */

#include "kernel/types.h"

// Simple VGA text mode for x86
static void vga_putc(char c) {
    static u16* vga = (u16*)0xB8000;
    static u32 pos = 0;
    
    if (c == '\n') {
        pos += 80 - (pos % 80);
        return;
    }
    
    vga[pos++] = (u16)c | 0x0700;
}

static void vga_puts(const char* s) {
    while (*s) vga_putc(*s++);
}

// UART for ARM - TODO: implement
static void uart_putc(char c) {
    (void)c;  // placeholder
}

static void uart_puts(const char* s) {
    (void)s;  // placeholder
}

void kernel_main(u32 magic, u32 addr) {
    // Clear screen first
    for (int i = 0; i < 80*25; i++) {
        ((u16*)0xB8000)[i] = 0x0720;
    }
    
    vga_puts("BLOOD_KERNEL v0.1 - booted\n");
    vga_puts("Magic: ");
    
    // Show magic number
    char hex[] = "0123456789ABCDEF";
    for (int i = 28; i >= 0; i -= 4) {
        vga_putc(hex[(magic >> i) & 0xF]);
    }
    
    vga_puts("\nReady for commit 2...\n");
    
    while (1) {
        __asm__ volatile("hlt");
    }
}
