/*
 * arch/rt1170/main_weak.c – RT1170 overrides
 */

#include "kernel/types.h"
const char *arch_name(void) { return "ARM-M7"; }
const char *mcu_name(void)  { return "RT1170"; }
