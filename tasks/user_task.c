/*
 * user_task.c - demo user ELF task
 */

#include "kernel/types.h"
#include "kernel/uart.h"

extern void uart_putc(char c);
extern void uart_puts(const char* s);

void task_main(void) {
    while (1) {
        uart_puts("user task alive\r\n");
        for (volatile int i = 0; i < 500000; i++);
    }
}
