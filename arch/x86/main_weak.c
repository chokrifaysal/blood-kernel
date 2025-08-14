/*
 * arch/x86/main_weak.c – x86 QEMU overrides
 */

#include "kernel/types.h"

const char *arch_name(void) { return "x86-32"; }
const char *mcu_name(void)  { return "QEMU-i686"; }
const char *boot_name(void) { return "RTC+COM+FDC"; }

void vga_init(void);
void ps2_kbd_init(void);
void pci_init(void);
void ata_init(void);
void idt_init(void);
void pic_init(void);
void paging_init(u32 memory_size);
void cpuid_init(void);
void enable_interrupts(void);
void rtc_init(void);
void serial_init(u8 port, u32 baud_rate, u8 data_bits, u8 stop_bits, u8 parity);
void floppy_init(void);
void x86_pc_demo_init(void);

void clock_init(void) {
    /* Initialize CPU identification */
    cpuid_init();

    /* Initialize interrupt system */
    idt_init();
    pic_init();

    /* Initialize memory management */
    paging_init(64 * 1024 * 1024); /* 64MB default */

    /* Initialize hardware */
    rtc_init();
    serial_init(0, 115200, 8, 1, 0); /* COM1: 115200 8N1 */
    serial_init(1, 9600, 8, 1, 0);   /* COM2: 9600 8N1 */
    floppy_init();

    /* Enable interrupts */
    pic_enable_irq(0); /* Timer */
    pic_enable_irq(1); /* Keyboard */
    pic_enable_irq(3); /* COM2 */
    pic_enable_irq(4); /* COM1 */
    pic_enable_irq(6); /* Floppy */
    pic_enable_irq(8); /* RTC */
    enable_interrupts();
}

void gpio_init(void) {
    /* No GPIO on PC - use PCI devices */
}

void ipc_init(void) {
    /* Single core for now */
}
