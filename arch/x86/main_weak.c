/*
 * arch/x86/main_weak.c – x86 QEMU overrides
 */

#include "kernel/types.h"

const char *arch_name(void) { return "x86-32"; }
const char *mcu_name(void)  { return "QEMU-i686"; }
const char *boot_name(void) { return "IDT+MMU+IRQ"; }

void vga_init(void);
void ps2_kbd_init(void);
void pci_init(void);
void ata_init(void);
void idt_init(void);
void pic_init(void);
void paging_init(u32 memory_size);
void cpuid_init(void);
void enable_interrupts(void);
void x86_pc_demo_init(void);

void clock_init(void) {
    /* Initialize CPU identification */
    cpuid_init();

    /* Initialize interrupt system */
    idt_init();
    pic_init();

    /* Initialize memory management */
    paging_init(64 * 1024 * 1024); /* 64MB default */

    /* Enable interrupts */
    pic_enable_irq(0); /* Timer */
    pic_enable_irq(1); /* Keyboard */
    enable_interrupts();
}

void gpio_init(void) {
    /* No GPIO on PC - use PCI devices */
}

void ipc_init(void) {
    /* Single core for now */
}
