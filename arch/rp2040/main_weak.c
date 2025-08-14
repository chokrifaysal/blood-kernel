/*
 * arch/rp2040/main_weak.c – RP2040 overrides
 */

#include "kernel/types.h"
const char *arch_name(void) { return "ARM-M0+"; }
const char *mcu_name(void)  { return "RP2040"; }
