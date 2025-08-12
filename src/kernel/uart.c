/*
 * uart.c - COM1 (0x3F8) for x86, USART1 for STM32F4
 */

#include "kernel/uart.h"
#include "kernel/types.h"

#ifdef __x86_64__            // actually i686, but gcc sets this
#define UART_BASE 0x3F8
#define UART_REG(r) (UART_BASE + (r))

static void uart_write_reg(u16 reg, u8 val) {
    *(volatile u8*)UART_REG(reg) = val;
}

static u8 uart_read_reg(u16 reg) {
    return *(volatile u8*)UART_REG(reg);
}

void uart_early_init(void) {
    uart_write_reg(3, 0x80);   // DLAB on
    uart_write_reg(0, 0x01);   // divisor 115200
    uart_write_reg(1, 0x00);
    uart_write_reg(3, 0x03);   // 8N1
    uart_write_reg(2, 0xC7);   // FIFO
}

void uart_putc(char c) {
    while (!(uart_read_reg(5) & 0x20));
    uart_write_reg(0, c);
}

#elif defined(__arm__)       // ARM Cortex-M4
#define USART1_BASE 0x40011000
#define RCC_BASE    0x40023800

#define RCC_AHB1ENR   (*(volatile u32*)(RCC_BASE + 0x30))
#define RCC_APB2ENR   (*(volatile u32*)(RCC_BASE + 0x44))
#define USART1_SR     (*(volatile u32*)(USART1_BASE + 0x00))
#define USART1_DR     (*(volatile u32*)(USART1_BASE + 0x04))
#define USART1_BRR    (*(volatile u32*)(USART1_BASE + 0x08))
#define USART1_CR1    (*(volatile u32*)(USART1_BASE + 0x0C))

void uart_early_init(void) {
    RCC_AHB1ENR |= (1<<0);   // GPIOA clock
    RCC_APB2ENR |= (1<<4);   // USART1 clock
    
    USART1_BRR = 0x0683;     // 84MHz/115200 = 0x0683
    USART1_CR1 = (1<<13) | (1<<3) | (1<<2);  // UE, TE, RE
}

void uart_putc(char c) {
    while (!(USART1_SR & (1<<7)));   // TXE
    USART1_DR = c;
}

#endif

void uart_puts(const char* s) {
    while (*s) uart_putc(*s++);
}

void uart_hex(u32 val) {
    const char hex[] = "0123456789ABCDEF";
    uart_puts("0x");
    for (int i = 28; i >= 0; i -= 4) {
        uart_putc(hex[(val >> i) & 0xF]);
    }
}
