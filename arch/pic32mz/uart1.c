/*
 * uart1.c – PIC32MZ UART1 115200 @ 50 MHz PBCLK
 */

#include "kernel/types.h"

#define UART1_BASE 0xBF806000UL

typedef volatile struct {
    u32 UxMODE;
    u32 UxSTA;
    u32 UxTXREG;
    u32 UxRXREG;
    u32 UxBRG;
} UART_TypeDef;

static UART_TypeDef* const UART1 = (UART_TypeDef*)UART1_BASE;

void uart_early_init(void) {
    /* 50 MHz / (16 * 115200) – 1 = 26 */
    UART1->UxBRG = 26;
    UART1->UxMODE = (1<<15); // UARTEN
    UART1->UxSTA  = (1<<10); // UTXEN
}

void uart_putc(char c) {
    while (!(UART1->UxSTA & (1<<9))); // UTXBF
    UART1->UxTXREG = c;
}

void uart_puts(const char* s) {
    while (*s) uart_putc(*s++);
}
