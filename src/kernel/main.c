/*
 * kernel_main – RA6M5 demo
 */

#ifdef __arm__ && defined(__ARM_ARCH_8M_MAIN__)
#include "arch/ra6m5/sci9_uart.c"
#include "arch/ra6m5/eth_mac.c"
#include "arch/ra6m5/usb_fs.c"
#include "arch/ra6m5/trustzone.c"
#endif

void kernel_main(void) {
#ifdef __ARM_ARCH_8M_MAIN__
    uart_early_init();
    kprintf("RA6M5 Cortex-M33 blood_kernel v1.7\r\n");

    tz_init();
    kprintf("TrustZone enabled\r\n");

    eth_init();
    usb_init();

    sched_init();
    task_create(idle_task, 0, 256);
    task_create(eth_task, 0, 512);
    task_create(usb_task, 0, 512);
    sched_start();
#endif
}
