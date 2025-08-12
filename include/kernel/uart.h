/*
 * uart.h - bare-metal UART0 for x86 COM1 and ARM USART1
 */

#ifndef _BLOOD_UART_H
#define _BLOOD_UART_H

void uart_early_init(void);
void uart_putc(char c);
void uart_puts(const char* s);
void uart_hex(u32 val);

#endif
