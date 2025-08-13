/*
 * clock.c – switch 24 MHz → 1 GHz PLL
 */

#include "kernel/types.h"

#define CCM_ANALOG_BASE 0x400D8000UL
#define PLL_ARM         (*(volatile u32*)(CCM_ANALOG_BASE + 0x60))

void clock_init(void) {
    /* 24 MHz * 42 = 1008 MHz */
    PLL_ARM = (1<<31) | (42<<0);
    while (!(PLL_ARM & (1<<31)));
}
