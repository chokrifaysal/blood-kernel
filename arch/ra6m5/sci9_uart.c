/*
 * sci9_uart.c – RA6M5 SCI9 UART0 @ 115200
 * 200 MHz PCLKA → 115200 → BRR = 108
 */

#include "kernel/types.h"

#define SCI9_BASE 0x40070000UL
#define PCLKA     200000000UL

typedef volatile struct {
    u32 SMR;
    u32 BRR;
    u32 SCR;
    u32 TDR;
    u32 SSR;
    u32 RDR;
} SCI_TypeDef;

static SCI_TypeDef* const SCI9 = (SCI_TypeDef*)SCI9_BASE;

void uart_early_init(void) {
    SCI9->SMR  = 0x00;                 // async 8-bit
    SCI9->BRR  = (PCLKA / (16UL * 115200UL)) - 1;
    SCI9->SCR  = (1<<5) | (1<<4);      // TE + RE
}

void uart_putc(char c) {
    while (!(SCI9->SSR & (1<<7)));     // TDRE
    SCI9->TDR = c;
}

void uart_puts(const char* s) {
    while (*s) uart_putc(*s++);
}
