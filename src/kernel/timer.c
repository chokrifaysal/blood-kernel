/*
 * timer.c - 1 ms tick source with critical sections
 */

#include "kernel/timer.h"
#include "kernel/types.h"
#include "kernel/spinlock.h"
#include "uart.h"

static volatile u32 system_ticks = 0;
static spinlock_t tick_lock = {0};

#ifdef __x86_64__
/* x86 PIT timer driver */
extern void pit_init(u32 frequency);
extern u32 pit_get_ticks(void);
extern void pit_delay(u32 ms);
extern void pit_irq_handler(void);

void timer_init(void) {
    pit_init(1000);
    uart_puts("PIT timer: 1 kHz\r\n");
}

u32 timer_ticks(void) {
    return pit_get_ticks();
}

void timer_delay(u32 ms) {
    pit_delay(ms);
}

void timer_irq_handler(void) {
    pit_irq_handler();
}

#elif defined(__arm__)
#define SYSTICK_BASE 0xE000E010
#define SYSTICK_CTRL   (*(volatile u32*)(SYSTICK_BASE + 0x00))
#define SYSTICK_LOAD   (*(volatile u32*)(SYSTICK_BASE + 0x04))
#define SYSTICK_VAL    (*(volatile u32*)(SYSTICK_BASE + 0x08))

void timer_init(void) {
    SYSTICK_LOAD = 10500 - 1;
    SYSTICK_VAL = 0;
    SYSTICK_CTRL = (1<<0) | (1<<1) | (1<<2);

    uart_puts("SysTick timer: 1 kHz\r\n");
}

u32 timer_ticks(void) {
    u32 ticks;
    __sync_synchronize();
    ticks = system_ticks;
    __sync_synchronize();
    return ticks;
}

void timer_delay(u32 ms) {
    u32 end = timer_ticks() + ms;
    while (timer_ticks() < end);
}

void SysTick_Handler(void) {
    __sync_fetch_and_add(&system_ticks, 1);
    if (system_ticks % 100 == 0) {
        *(volatile u32*)0xE000ED04 = (1<<28);  // PendSV
    }
}

#elif defined(__AVR_ARCH__)
// AVR timer
extern void timer_init(void);
extern u32 timer_ticks(void);
extern void timer_delay(u32 ms);

#elif defined(__RP2040__)
// RP2040 timer in ASF
extern void timer_init(void);
extern u32 timer_ticks(void);
extern void timer_delay(u32 ms);

#else
// stub implementations
void timer_init(void) { }
u32 timer_ticks(void) { return 0; }
void timer_delay(u32 ms) { (void)ms; }

#endif
