/*
 * arch/tms570/main_weak.c – TMS570 overrides
 */

#include "kernel/types.h"
const char *arch_name(void) { return "ARM-R4F"; }
const char *mcu_name(void)  { return "TMS570LS3137"; }
