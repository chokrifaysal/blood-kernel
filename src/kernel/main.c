/*
 * kernel_main – GD32VF103 demo
 */

#ifdef __riscv && defined(__riscv_xlen) && __riscv_xlen == 32
#include "arch/gd32vf103/clock.c"
#include "arch/gd32vf103/uart0.c"
#include "arch/gd32vf103/can.c"
#include "arch/gd32vf103/qspi.c"
#include "arch/gd32vf103/usb_fs_500loc.c"
#endif

void kernel_main(void) {
#ifdef __riscv_xlen
    clock_init();
    uart_early_init();
    kprintf("GD32VF103 RISC-V @108 MHz\r\n");

    can_init();
    qspi_init();
    usb_init();

    sched_init();
    task_create(idle_task, 0, 256);
    task_create(can_task, 0, 256);
    task_create(usb_task, 0, 512);
    task_create(qspi_task, 0, 512);
    sched_start();
#endif
}
