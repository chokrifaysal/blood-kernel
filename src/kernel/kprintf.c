/*
 * kprintf.c - minimal vsnprintf for kernel
 */

#include "kernel/kprintf.h"
#include "kernel/uart.h"
#include "kernel/types.h"

static void print_hex(u32 val) {
    const char hex[] = "0123456789ABCDEF";
    for (int i = 28; i >= 0; i -= 4) {
        uart_putc(hex[(val >> i) & 0xF]);
    }
}

static void print_dec(u32 val) {
    char buf[12];
    int idx = 0;
    
    if (val == 0) {
        uart_putc('0');
        return;
    }
    
    while (val) {
        buf[idx++] = '0' + (val % 10);
        val /= 10;
    }
    
    while (idx--) {
        uart_putc(buf[idx]);
    }
}

void kprintf(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    
    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt++) {
                case 'x': print_hex(va_arg(ap, u32)); break;
                case 'd': print_dec(va_arg(ap, u32)); break;
                case 's': {
                    const char* s = va_arg(ap, const char*);
                    while (*s) uart_putc(*s++);
                    break;
                }
                case 'c': uart_putc(va_arg(ap, int)); break;
                default: uart_putc('%'); fmt--; break;
            }
        } else {
            uart_putc(*fmt++);
        }
    }
    
    va_end(ap);
}
