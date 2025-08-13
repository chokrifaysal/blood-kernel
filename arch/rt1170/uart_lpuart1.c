/*
 * uart_lpuart1.c – LPUART1 @ 1 GHz 115200
 */

#include "kernel/types.h"

#define LPUART1_BASE 0x4018C000UL
#define LPUART_BAUD  0x0055020AUL /* 1 GHz / 115200 */

typedef volatile struct {
    u32 BAUD;
    u32 STAT;
    u32 CTRL;
    u32 DATA;
} LPUART_TypeDef;

static LPUART_TypeDef* const LPUART1 = (LPUART_TypeDef*)LPUART1_BASE;

void uart_early_init(void) {
    LPUART1->BAUD = LPUART_BAUD;
    LPUART1->CTRL = (1<<19) | (1<<18); // TE + RE
}

void uart_putc(char c) {
    while (!(LPUART1->STAT & (1<<23))); // TDRE
    LPUART1->DATA = c;
}

void uart_puts(const char* s) {
    while (*s) uart_putc(*s++);
}
