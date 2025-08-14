/*
 * arch/stm32h/main_weak.c – STM32H745 overrides
 */

#include "kernel/types.h"
const char *arch_name(void) { return "ARM-M7+M4"; }
const char *mcu_name(void)  { return "STM32H745"; }
