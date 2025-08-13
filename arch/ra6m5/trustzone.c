/*
 * trustzone.c – RA6M5 TrustZone SAU setup
 */

#include "kernel/types.h"

#define SAU_BASE 0xE000EDD0UL

static volatile u32* const SAU_CTRL = (u32*)SAU_BASE;
static volatile u32* const SAU_RNR  = (u32*)(SAU_BASE + 0x4);
static volatile u32* const SAU_RLAR = (u32*)(SAU_BASE + 0xC);

void tz_init(void) {
    /* region 0: RAM 0x20000000-0x2007FFFF secure */
    *SAU_RNR = 0;
    *(SAU_RLAR) = 0x2007FFFF | (1<<0); // enable
    *SAU_CTRL = (1<<0);                // SAU enable
}
