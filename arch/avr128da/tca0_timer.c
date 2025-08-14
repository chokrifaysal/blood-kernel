/*
 * tca0_timer.c – AVR128DA48 TCA0 16-bit timer @ 20 MHz → 1 kHz tick
 */

#include "kernel/types.h"
#include <avr/io.h>

static volatile u32 sys_ticks = 0;

void timer_init(void) {
    /* TCA0 setup: 20 MHz / 64 / 312 = 1.002 kHz ≈ 1 ms */
    TCA0.SINGLE.CTRLA = 0;                    // stop
    TCA0.SINGLE.CTRLB = 0;                    // normal mode
    TCA0.SINGLE.PER = 312;                    // period
    TCA0.SINGLE.INTCTRL = (1<<0);             // OVF int enable
    TCA0.SINGLE.CTRLA = (3<<1) | (1<<0);     // div64 + enable
}

u32 timer_ticks(void) {
    u32 ticks;
    __asm__ volatile("cli");
    ticks = sys_ticks;
    __asm__ volatile("sei");
    return ticks;
}

void timer_delay(u32 ms) {
    u32 end = timer_ticks() + ms;
    while (timer_ticks() < end);
}

void tca0_tick_handler(void) {
    TCA0.SINGLE.INTFLAGS = (1<<0);            // clear OVF flag
    sys_ticks++;
}

/* PWM output on PA0 (WO0) */
void pwm_init(void) {
    PORTA.DIRSET = (1<<0);                    // PA0 output
    TCA0.SINGLE.CTRLB = (1<<4);               // WO0 enable
}

void pwm_set(u8 duty) {
    TCA0.SINGLE.CMP0 = (u16)duty * 312 / 255;
}
