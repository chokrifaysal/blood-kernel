/*
 * uart0.c – AVR128DA48 USART0 @ 20 MHz 115200 8N1
 */

#include "kernel/types.h"
#include <avr/io.h>

void uart_early_init(void) {
    /* 20 MHz → 115200 → UBRR = 10 */
    USART0.BAUD = 10;
    USART0.CTRLA = 0;
    USART0.CTRLB = (1<<TXEN0) | (1<<RXEN0);
    USART0.CTRLC = (3<<UCSZ00); // 8N1
}

void uart_putc(char c) {
    while (!(USART0.STATUS & (1<<DRE0)));
    USART0.TXDATAL = c;
}

void uart_puts(const char* s) {
    while (*s) uart_putc(*s++);
}
