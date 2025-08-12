/*
 * kernel_main – AVR128DA48 demo
 */

#ifdef __AVR_ARCH__
#include "arch/avr128da/uart0.c"
#include "arch/avr128da/spi_master.c"
#include "arch/avr128da/twi_slave.c"
#include "arch/avr128da/gpio_5v.c"
#endif

void kernel_main(void) {
#ifdef __AVR_ARCH__
    uart_early_init();
    kprintf("AVR128DA48 blood_kernel v1.5\r\n");

    spi_init();
    twi_init(0x42);         // TWI slave addr 0x42
    gpio_set_dir(7, 1);     // LED pin
    gpio_put(7, 1);

    sched_init();
    task_create(idle_task, 0, 128);
    task_create(spi_task, 0, 128);
    task_create(twi_task, 0, 128);
    sched_start();
#endif
}
