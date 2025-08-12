/*
 * gicv2.c - minimal GICv2 init
 */

#include "gicv2.h"
#include "uart.h"

static volatile u32* const GICD = (u32*)GICD_BASE;
static volatile u32* const GICC = (u32*)GICC_BASE;

void gic_init(void) {
    // Distributor
    for (u32 i = 0; i < 32; i++) {
        GICD[i] = 0;                     // disable all
    }
    GICD[0] = 1;                         // enable distributor
    
    // CPU interface
    GICC[0] = 1;                         // enable
}

void gic_enable_irq(u32 irq) {
    GICD[irq/32] |= (1 << (irq % 32));
}

void gic_eoi(u32 irq) {
    (void)irq;
    GICC[1] = 0;                         // EOI
}
