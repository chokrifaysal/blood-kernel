/*
 * gpio_sio.c - SIO GPIO bit-bang
 */

#include "gpio_sio.h"
#include "kernel/types.h"

#define SIO_BASE 0xD0000000UL
#define GPIO_OUT   (*(volatile u32*)(SIO_BASE + 0x010))
#define GPIO_OE    (*(volatile u32*)(SIO_BASE + 0x020))

void gpio_set_dir(u8 pin, u8 out) {
    if (out) GPIO_OE |= (1<<pin);
    else     GPIO_OE &= ~(1<<pin);
}

void gpio_put(u8 pin, u8 val) {
    if (val) GPIO_OUT |= (1<<pin);
    else     GPIO_OUT &= ~(1<<pin);
}

void gpio_toggle(u8 pin) {
    GPIO_OUT ^= (1<<pin);
}
