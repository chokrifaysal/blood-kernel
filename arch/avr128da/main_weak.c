/*
 * arch/avr128da/main_weak.c – AVR128DA overrides
 */

#include "kernel/types.h"

const char *arch_name(void) { return "AVR8"; }
const char *mcu_name(void)  { return "AVR128DA48"; }
const char *boot_name(void) { return "TCA0"; }

void clock_init(void) {
    /* 20 MHz internal osc already running */
}

void gpio_init(void) {
    /* PA0 = PWM out, PB0-7 = 5V GPIO */
    PORTA.DIRSET = (1<<0);
    PORTB.DIRSET = 0xFF;
}
