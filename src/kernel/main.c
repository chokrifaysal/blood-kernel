/*
 * kernel_main
 * Both x86 and ARM entry point
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

// tiny demo task in flash
static const u8 demo_elf[] = {
    0x7F, 'E', 'L', 'F', 1, 1, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0x02, 0x00, 0x28, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x10, 0x01, 0x00, 0x34, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x34, 0x00, 0x20, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x01, 0x00,
    0x00, 0x10, 0x01, 0x00, 0x05, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
};

void kernel_main(u32 magic, u32 addr) {
    uart_early_init();
    
    uart_puts("\r\n");
    uart_puts("BLOOD_KERNEL v0.7 - booted\r\n");
    
    if (magic == 0x2BADB002) {
        detect_memory_x86(addr);
    } else {
        detect_memory_arm();
    }
    
    sched_init();
    timer_init();
    gpio_init();
    msg_init(&uart_rx_q);
    can_init(500000);
    
#ifdef __arm__
    mpu_init();   // ARM only
#endif
    
    // load demo ELF task
    u32 entry;
    if (elf_load(demo_elf, &entry)) {
        task_create((task_entry_t)entry, 0, 512);
    }
    
    task_create(idle_task, 0, 256);
    task_create(blink_task, 0, 256);
    task_create(can_task, 0, 512);
    
    uart_puts("Starting scheduler...\r\n");
    sched_start();
    
    while (1) {
        __asm__ volatile("hlt");
    }
}

void idle_task(void) {
    while (1) {
        __asm__ volatile("nop");
    }
}

void blink_task(void) {
    gpio_set_mode(PA5, GPIO_OUT);
    
    while (1) {
        gpio_toggle(PA5);
        timer_delay(200);
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

// demo task linked at 0x10010000
void demo_task(void) {
    while (1) {
        uart_puts("Hello from ELF!\r\n");
        timer_delay(500);
    }
}
