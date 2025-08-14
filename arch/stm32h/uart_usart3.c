/*
 * uart_usart3.c – shared UART3, 115200 8N1
 */

#include "kernel/types.h"

#define USART3_BASE 0x40004800UL
#define RCC_APB1ENR (*(volatile u32*)0x58024440UL)

void uart_early_init(void) {
    RCC_APB1ENR |= (1<<18); // USART3EN
    *(volatile u32*)(USART3_BASE + 0x0C) = 0x1B2; // 480e6 / 115200
    *(volatile u32*)(USART3_BASE + 0x08) = (1<<13) | (1<<3) | (1<<2);
}

void uart_putc(char c) {
    while (!(*(volatile u32*)(USART3_BASE + 0x1C) & (1<<7)));
    *(volatile u32*)(USART3_BASE + 0x28) = c;
}
