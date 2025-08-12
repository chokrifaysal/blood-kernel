/*
 * uart0_esp32s3.c – 115200 8N1, APB = 80 MHz
 */

#include "kernel/types.h"

#define UART0_BASE 0x60000000UL
#define GPIO_BASE  0x60004000UL

typedef volatile struct {
    u32 FIFO;
    u32 INT_RAW;
    u32 INT_ENA;
    u32 CLKDIV;
    u32 CONF0;
    u32 CONF1;
} UART_TypeDef;

static UART_TypeDef* const UART = (UART_TypeDef*)UART0_BASE;

void uart_early_init(void) {
    // GPIO matrix: TX=43, RX=44
    *(u32*)(GPIO_BASE + 0x14) = (43 << 8) | 0x100; // TX
    *(u32*)(GPIO_BASE + 0x18) = (44 << 8) | 0x100; // RX

    // 80 MHz / (115200 * 16) ≈ 43.4
    UART->CLKDIV = 43 << 20;
    UART->CONF0  = (1<<2) | (1<<1); // 8N1
    UART->CONF1  = 0;
}

void uart_putc(char c) {
    while (!(UART->INT_RAW & (1<<1))); // TXFIFO_EMPTY
    UART->FIFO = c;
}
