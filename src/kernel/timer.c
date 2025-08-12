/*
 * timer.c - 1 ms tick source
 */

#include "kernel/timer.h"
#include "kernel/types.h"
#include "uart.h"

static volatile u32 system_ticks = 0;

#ifdef __x86_64__
#define PIT_CH0  0x40
#define PIT_CMD  0x43
#define PIT_FREQ 1193182

static void pit_init(void) {
    // set PIT channel 0 to 1 kHz
    u32 divisor = PIT_FREQ / 1000;
    
    outb(PIT_CMD, 0x36);        // channel 0, lobyte/hibyte, rate gen
    outb(PIT_CH0, divisor & 0xFF);
    outb(PIT_CH0, divisor >> 8);
}

static u8 inb(u16 port) {
    u8 val;
    __asm__ volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static void outb(u16 port, u8 val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

void timer_init(void) {
    pit_init();
    uart_puts("PIT timer: 1 kHz\r\n");
}

u32 timer_ticks(void) {
    return system_ticks;
}

void timer_delay(u32 ms) {
    u32 end = system_ticks + ms;
    while (system_ticks < end);
}

#elif defined(__arm__)
#define SYSTICK_BASE 0xE000E010
#define SYSTICK_CTRL   (*(volatile u32*)(SYSTICK_BASE + 0x00))
#define SYSTICK_LOAD   (*(volatile u32*)(SYSTICK_BASE + 0x04))
#define SYSTICK_VAL    (*(volatile u32*)(SYSTICK_BASE + 0x08))
#define SYSTICK_CALIB  (*(volatile u32*)(SYSTICK_BASE + 0x0C))

void timer_init(void) {
    // 84 MHz / 8 = 10.5 MHz -> reload 10500 for 1 kHz
    SYSTICK_LOAD = 10500 - 1;
    SYSTICK_VAL = 0;
    SYSTICK_CTRL = (1<<0) | (1<<1) | (1<<2);  // enable, int, clk/8
    
    uart_puts("SysTick timer: 1 kHz\r\n");
}

u32 timer_ticks(void) {
    return system_ticks;
}

void timer_delay(u32 ms) {
    u32 end = system_ticks + ms;
    while (system_ticks < end);
}

// SysTick ISR - keep it minimal
void SysTick_Handler(void) {
    system_ticks++;
    if (system_ticks % 100 == 0) {
        // trigger PendSV every 100 ms for preemption
        *(volatile u32*)0xE000ED04 = (1<<28);
    }
}

#endif
