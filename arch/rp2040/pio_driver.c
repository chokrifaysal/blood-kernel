/*
 * pio_driver.c – load & run PIO blink
 */

#include "pio_driver.h"
#include "kernel/types.h"

#define PIO0_BASE 0x50200000UL

void pio_load_blink(u8 pin) {
    *(volatile u32*)(PIO0_BASE + 0x00) = 0x00000000; // reset
    *(volatile u32*)(PIO0_BASE + 0x0C) = 0x00000000; // clkdiv = 1
    *(volatile u32*)(PIO0_BASE + 0x10) = (pin<<24) | (1<<19); // set pin
    *(volatile u32*)(PIO0_BASE + 0x14) = 0x00000000; // FIFO
    *(volatile u32*)(PIO0_BASE + 0x18) = 0x00000000; // SM enable
}
