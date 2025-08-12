/*
 * kernel_main – now supports RPi4
 */

#include "kernel/types.h"
#include "uart.h"
#include "kernel/sched.h"
#include "kernel/timer.h"
#include "kernel/gpio.h"
#include "kernel/can.h"
#include "kernel/wdog.h"

#ifdef __aarch64__
#include "arch/rpi4/gicv2.h"
#include "arch/rpi4/pcie_stub.h"
#endif

void kernel_main(u32 magic, u32 addr) {
    uart_early_init();
    kprintf("BLOOD_KERNEL v1.2 – RPi4 boot\r\n");

#ifdef __aarch64__
    gic_init();
    kprintf("GICv2 up\r\n");
    kprintf("PCIe VID=0x%x\r\n", pcie_read(0,0,0,0));
#endif

    sched_init();
    timer_init();
    task_create(idle_task, 0, 256);
    sched_start();
}
