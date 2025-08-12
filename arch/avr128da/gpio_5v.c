/*
 * gpio_5v.c – 5 V tolerant GPIO on PORTB
 */

#include "kernel/types.h"
#include <avr/io.h>

void gpio_set_dir(u8 pin, u8 out) {
    if (out) PORTB.DIRSET = (1<<pin);
    else     PORTB.DIRCLR = (1<<pin);
}

void gpio_put(u8 pin, u8 val) {
    if (val) PORTB.OUTSET = (1<<pin);
    else     PORTB.OUTCLR = (1<<pin);
}

void gpio_toggle(u8 pin) {
    PORTB.OUTTGL = (1<<pin);
}
