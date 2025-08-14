/*
 * clock.c – 480 MHz on CM7, 240 MHz on CM4
 */

#include "kernel/types.h"

void clock_init(void) {
    /* RCC->PLLCFGR = 480 MHz HCLK via PLL1 & PLL2 */
    *(volatile u32*)0x58024410 = 0x1A0A0C01;
    *(volatile u32*)0x58024400 |= (1<<24); // PLLON
    while (!(*(volatile u32*)0x58024400 & (1<<25)));
}
