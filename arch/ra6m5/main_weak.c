/*
 * arch/ra6m5/main_weak.c – RA6M5 overrides
 */

#include "kernel/types.h"
const char *arch_name(void) { return "ARM-M33"; }
const char *mcu_name(void)  { return "RA6M5"; }
