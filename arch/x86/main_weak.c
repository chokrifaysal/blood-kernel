/*
 * arch/x86/main_weak.c – x86 QEMU overrides
 */

#include "kernel/types.h"

const char *arch_name(void) { return "x86-32"; }
const char *mcu_name(void)  { return "QEMU-i686"; }
const char *boot_name(void) { return "VGA+PCI+ATA"; }

void vga_init(void);
void ps2_kbd_init(void);
void pci_init(void);
void ata_init(void);
void x86_pc_demo_init(void);

void clock_init(void) {
    /* CPU clock already configured by BIOS */
}

void gpio_init(void) {
    /* No GPIO on PC - use PCI devices */
}

void ipc_init(void) {
    /* Single core for now */
}
