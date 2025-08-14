/*
 * arch/gd32vf103/main_weak.c – GD32VF103 overrides
 */

#include "kernel/types.h"
const char *arch_name(void) { return "RISC-V"; }
const char *mcu_name(void)  { return "GD32VF103"; }
