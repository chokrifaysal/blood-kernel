/*
 * arch/rp2040/main_weak.c – RP2040 overrides
 */

#include "kernel/types.h"

const char *arch_name(void) { return "ARM-M0+"; }
const char *mcu_name(void)  { return "RP2040"; }
const char *boot_name(void) { return "PIO+Timer"; }

void timer_init(void);
void pio_init(void);
void flash_init(void);
void multicore_launch_core1(void (*entry)(void));

void clock_init(void) {
    /* 125 MHz system clock already configured by bootrom */
}

void gpio_init(void) {
    /* GPIO SIO already initialized */
}

void ipc_init(void) {
    /* Dual-core FIFO communication ready */
}
