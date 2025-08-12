/*
 * gicv2.h - GICv2 distributor + CPU interface
 */

#ifndef _BLOOD_GICV2_H
#define _BLOOD_GICV2_H

#include "kernel/types.h"

#define GICD_BASE 0xFF841000UL
#define GICC_BASE 0xFF842000UL

void gic_init(void);
void gic_enable_irq(u32 irq);
void gic_eoi(u32 irq);

#endif
