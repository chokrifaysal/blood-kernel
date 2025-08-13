/*
 * clock.c – switch HIRC 8 MHz → PLL 108 MHz
 */

#include "kernel/types.h"

#define RCU_BASE 0x40021000UL
#define RCU_CTL  (*(volatile u32*)(RCU_BASE + 0x00))
#define RCU_CFG0 (*(volatile u32*)(RCU_BASE + 0x04))

void clock_init(void) {
    /* 8 MHz * 27 = 216 → /2 = 108 MHz */
    RCU_CFG0 = (2<<0) | (15<<18); // PLLSRC=HIRC, PLLMUL=27
    RCU_CTL |= (1<<24);           // PLLEN
    while (!(RCU_CTL & (1<<25))); // PLLRDY
    RCU_CFG0 |= (2<<0);           // SYSCLK=PLL
}
