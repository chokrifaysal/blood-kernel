/*
 * arch/pic32mz/main_weak.c – PIC32MZ overrides
 */

#include "kernel/types.h"
const char *arch_name(void) { return "MIPS32"; }
const char *mcu_name(void)  { return "PIC32MZ2048EFH"; }
