/*
 * uart_pl011.c - BCM2711 mini UART 0 (PL011)
 */

#include "kernel/types.h"

#define UART0_BASE 0xFE201000UL

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
    u32 IFLS;
    u32 IMSC;
    u32 RIS;
    u32 MIS;
    u32 ICR;
} PL011_TypeDef;

static PL011_TypeDef* const UART0 = (PL011_TypeDef*)UART0_BASE;

void uart_early_init(void) {
    // 115200 @ 48 MHz
    UART0->CR = 0;
    UART0->IBRD = 26;
    UART0->FBRD = 3;
    UART0->LCRH = (3<<5);   // 8N1
    UART0->CR = (1<<0) | (1<<8) | (1<<9);
}

void uart_putc(char c) {
    while (UART0->FR & (1<<5));   // TXFF
    UART0->DR = c;
}

void uart_puts(const char* s) {
    while (*s) uart_putc(*s++);
}
