/*
 * arch/x86/main_weak.c – x86 QEMU overrides
 */

#include "kernel/types.h"
const char *arch_name(void) { return "x86-32"; }
const char *mcu_name(void)  { return "QEMU-i686"; }
