/*
 * kernel_main – PIC32MZ demo
 */

#ifdef __mips__
#include "arch/pic32mz/uart1.h"
#include "arch/pic32mz/canfd.h"
#include "arch/pic32mz/qspi_flash.h"
#endif

void kernel_main(void) {
#ifdef __mips__
    uart_early_init();
    kprintf("PIC32MZ2048EFH blood_kernel v1.4\r\n");

    qspi_init();
    qspi_erase_sector(0x00080000);
    kprintf("QSPI erased @ 0x80000\r\n");

    canfd_init(1000000); // 1 Mbit/s
    kprintf("CAN-FD up\r\n");

    sched_init();
    task_create(idle_task, 0, 256);
    task_create(canfd_task, 0, 512);
    task_create(qspi_task, 0, 512);
    sched_start();
#endif
}
