/*
 * uart0.c - RP2040 UART0 @ 115200 (48 MHz clk)
 */

#include "kernel/types.h"

#define UART0_BASE 0x40034000UL
#define RESETS_BASE 0x4000C000UL
#define CLOCKS_BASE 0x40008000UL

typedef volatile struct {
    u32 DR;
    u32 RSRECR;
    u32 _pad1[4];
    u32 FR;
    u32 _pad2[2];
    u32 IBRD;
    u32 FBRD;
    u32 LCRH;
    u32 CR;
} UART0_TypeDef;

static UART0_TypeDef* const UART = (UART0_TypeDef*)UART0_BASE;

void uart_early_init(void) {
    // reset UART0
    *(u32*)(RESETS_BASE + 0x0) &= ~(1<<22);
    while (*(u32*)(RESETS_BASE + 0x0) & (1<<22));

    // 115200 @ 48 MHz → 26 + 3
    UART->IBRD = 26;
    UART->FBRD = 3;
    UART->LCRH = (3<<5);   // 8N1
    UART->CR   = (1<<0) | (1<<8) | (1<<9);
}

void uart_putc(char c) {
    while (UART->FR & (1<<5));   // TXFF
    UART->DR = c;
}

void uart_puts(const char* s) {
    while (*s) uart_putc(*s++);
}
