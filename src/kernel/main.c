/*
 * kernel_main - final automotive release
 */

#include "kernel/types.h"
#include "kernel/uart.h"
#include "kernel/mem.h"
#include "kernel/sched.h"
#include "kernel/timer.h"
#include "kernel/gpio.h"
#include "kernel/spinlock.h"
#include "kernel/msg.h"
#include "kernel/can.h"
#include "kernel/mpu.h"
#include "kernel/elf.h"
#include "kernel/wdog.h"
#include "kernel/kprintf.h"
#include "kernel/spi.h"
#include "kernel/sd.h"
#include "kernel/log.h"

static const char banner[] =
    "BLOOD_KERNEL v1.0 2025-08-13 Automotive SIL-2\r\n";

void kernel_main(u32 magic, u32 addr) {
    uart_early_init();
    kprintf("%s", banner);
    
    // board detection
#if defined(__TI_TMS570__)
    kprintf("Board: TMS570LS3137\r\n");
#elif defined(__arm__)
    kprintf("Board: STM32F407\r\n");
#else
    kprintf("Board: x86 QEMU\r\n");
#endif
    
    // bootstrap
    sched_init();
    timer_init();
    gpio_init();
    msg_init(&uart_rx_q);
    can_init(500000);
    wdog_init(2000);
    log_init();
    
#ifdef __arm__
    mpu_init();
#endif
    
    // load safety task
    u32 entry;
    if (elf_load((const u8*)0x08080000, &entry)) {
        task_create((task_entry_t)entry, 0, 1024);
    }
    
    task_create(idle_task, 0, 256);
    task_create(blink_task, 0, 256);
    task_create(can_task, 0, 512);
    task_create(logger_task, 0, 512);
    task_create(safety_task, 0, 512);
    
    kprintf("All systems nominal, starting scheduler\r\n");
    sched_start();
}

void idle_task(void) {
    while (1) {
        wdog_refresh();
        task_yield();
    }
}

void blink_task(void) {
    gpio_set_mode(PA5, GPIO_OUT);
    while (1) {
        gpio_toggle(PA5);
        timer_delay(500);
    }
}

void can_task(void) {
    can_frame_t tx = {.id = 0x123, .len = 8, .data = {0xAA}};
    while (1) {
        tx.data[0]++;
        can_send(&tx);
        timer_delay(1000);
    }
}

void logger_task(void) {
    can_frame_t rx;
    while (1) {
        if (can_recv(&rx) == 0) {
            log_can(&rx);
        }
        timer_delay(50);
    }
}

void safety_task(void) {
    u32 can_rx_last = timer_ticks();
    while (1) {
        if (timer_ticks() - can_rx_last > 200) {
            kprintf("CAN RX timeout, rebooting\r\n");
            wdog_force_reset();
        }
        can_rx_last = timer_ticks();
        timer_delay(100);
    }
}
