/*
 * kernel_main – RT1170 1 GHz demo
 */

#ifdef __arm__ && defined(__ARM_ARCH_8M_MAIN__)
#include "arch/rt1170/clock.c"
#include "arch/rt1170/uart_lpuart1.c"
#include "arch/rt1170/enet_500loc.c"
#include "arch/rt1170/sdmmc.c"
#include "arch/rt1170/usb_hs.c"
#endif

void kernel_main(void) {
#ifdef __ARM_ARCH_8M_MAIN__
    clock_init();
    uart_early_init();
    kprintf("RT1170 M7 @1 GHz\r\n");

    enet_init();
    sdmmc_init();
    usb_hs_init();

    sched_init();
    task_create(idle_task, 0, 256);
    task_create(enet_task, 0, 512);
    task_create(sdmmc_task, 0, 512);
    task_create(usb_task, 0, 512);
    sched_start();
#endif
}
