/*
 * arch/esp32s3/main_weak.c – ESP32-S3 overrides
 */

#include "kernel/types.h"
const char *arch_name(void) { return "RISC-V"; }
const char *mcu_name(void)  { return "ESP32-S3"; }
