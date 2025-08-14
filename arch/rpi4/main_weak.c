/*
 * arch/rpi4/main_weak.c – Raspberry Pi 4 overrides
 */

#include "kernel/types.h"
const char *arch_name(void) { return "ARM-A72"; }
const char *mcu_name(void)  { return "BCM2711"; }
