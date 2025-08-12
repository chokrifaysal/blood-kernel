/*
 * kernel_main – RP2040 demo
 */

#include "kernel/types.h"
#include "kernel/sched.h"
#include "kernel/timer.h"
#include "arch/rp2040/uart0.h"
#include "arch/rp2040/gpio_sio.h"
#include "arch/rp2040/pio_driver.h"

void kernel_main(void) {
    uart_early_init();
    kprintf("RP2040 blood_kernel v1.3\r\n");
    
    gpio_set_dir(25, 1);          // onboard LED
    pio_load_blink(25);           // PIO blink
    
    sched_init();
    task_create(idle_task, 0, 256);
    sched_start();
}
