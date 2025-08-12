/*
 * kernel_main – ESP32-S3 RISC-V demo
 */

#ifdef __riscv
#include "arch/esp32s3/uart0_esp32s3.h"
#include "arch/esp32s3/gpio_matrix.h"
#include "arch/esp32s3/wifi_stub.h"
#include "arch/esp32s3/rtc_slow.h"
#endif

void kernel_main(void) {
#ifdef __riscv
    uart_early_init();
    kprintf("ESP32-S3 RISC-V boot OK\r\n");

    gpio_set_dir(2, 1);   // onboard LED
    gpio_toggle(2);

    wifi_init();
    kprintf("Wi-Fi MAC stub ready\r\n");

    rtc_store(0, 0xDEADBEEF);
    kprintf("RTC slow mem test: %08x\r\n", rtc_load(0));

    sched_init();
    task_create(idle_task, 0, 256);
    sched_start();
#endif
}
