/*
 * uart0.c – USART0 @ 115200 8N1, APB 54 MHz
 */

#include "kernel/types.h"

#define USART0_BASE 0x40013800UL
#define RCU_APB2ENR (*(volatile u32*)(0x40021000UL + 0x18))

void uart_early_init(void) {
    RCU_APB2ENR |= (1<<14); // USART0EN
    *(volatile u32*)(USART0_BASE + 0x0C) = 468; // 54e6 / 115200
    *(volatile u32*)(USART0_BASE + 0x08) = (1<<13) | (1<<2) | (1<<3); // TXEN+RXEN
}

void uart_putc(char c) {
    while (!(*(volatile u32*)(USART0_BASE + 0x1C) & (1<<7)));
    *(volatile u32*)(USART0_BASE + 0x28) = c;
}
