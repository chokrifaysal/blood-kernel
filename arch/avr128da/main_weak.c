/*
 * arch/avr128da/main_weak.c – AVR128DA overrides
 */

#include "kernel/types.h"
const char *arch_name(void) { return "AVR8"; }
const char *mcu_name(void)  { return "AVR128DA48"; }
