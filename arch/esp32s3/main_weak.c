/*
 * arch/esp32s3/main_weak.c – ESP32-S3 overrides
 */

#include "kernel/types.h"

const char *arch_name(void) { return "RISC-V"; }
const char *mcu_name(void)  { return "ESP32-S3"; }
const char *boot_name(void) { return "WiFi6+WPA3"; }

void wifi_init(void);
void esp32s3_wifi_demo_init(void);

void clock_init(void) {
    /* 240 MHz CPU, 80 MHz APB already configured */
}

void gpio_init(void) {
    /* GPIO matrix already initialized */
}

void ipc_init(void) {
    /* Dual-core IPC via FreeRTOS queues */
}
