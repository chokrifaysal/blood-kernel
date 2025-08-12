/*
 * wdog.c - STM32 IWDG with 32 kHz LSI
 */

#include "kernel/wdog.h"
#include "kernel/types.h"
#include "uart.h"

#ifdef __arm__

#define IWDG_BASE 0x40003000
#define IWDG_KR   (*(volatile u32*)(IWDG_BASE + 0x00))
#define IWDG_PR   (*(volatile u32*)(IWDG_BASE + 0x04))
#define IWDG_RLR  (*(volatile u32*)(IWDG_BASE + 0x08))
#define IWDG_SR   (*(volatile u32*)(IWDG_BASE + 0x0C))
#define RCC_CSR   (*(volatile u32*)0x40023874)

void wdog_init(u32 timeout_ms) {
    // enable LSI
    RCC_CSR |= (1<<0);
    while (!(RCC_CSR & (1<<1)));
    
    // prescaler 256 -> 32kHz/256 = 125 Hz
    IWDG_KR = 0x5555;
    IWDG_PR = 0x06;
    
    // reload = timeout * 125
    u32 reload = timeout_ms * 125 / 1000;
    if (reload > 0xFFF) reload = 0xFFF;
    
    IWDG_KR = 0x5555;
    IWDG_RLR = reload;
    
    // start
    IWDG_KR = 0xCCCC;
    
    uart_puts("IWDG started ");
    uart_hex(timeout_ms);
    uart_puts(" ms\r\n");
}

void wdog_refresh(void) {
    IWDG_KR = 0xAAAA;
}

void wdog_force_reset(void) {
    IWDG_KR = 0x5555;
    IWDG_RLR = 0x0001;
    IWDG_KR = 0xCCCC;
    while (1);   // wait for reset
}

#else
// x86 - stubs
void wdog_init(u32 timeout_ms) { (void)timeout_ms; }
void wdog_refresh(void) {}
void wdog_force_reset(void) {}

#endif
