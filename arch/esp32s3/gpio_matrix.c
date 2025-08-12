/*
 * gpio_matrix.c – bare-metal GPIO via GPIO_OUT_REG
 */

#include "gpio_matrix.h"
#include "kernel/types.h"

#define GPIO_BASE 0x60004000UL

static volatile u32* const GPIO_ENABLE = (u32*)(GPIO_BASE + 0x8);
static volatile u32* const GPIO_OUT    = (u32*)(GPIO_BASE + 0x4);

void gpio_set_dir(u8 pin, u8 out) {
    if (out) *GPIO_ENABLE |=  (1<<pin);
    else     *GPIO_ENABLE &= ~(1<<pin);
}

void gpio_set_level(u8 pin, u8 val) {
    if (val) *GPIO_OUT |=  (1<<pin);
    else     *GPIO_OUT &= ~(1<<pin);
}

void gpio_toggle(u8 pin) {
    *GPIO_OUT ^= (1<<pin);
}
