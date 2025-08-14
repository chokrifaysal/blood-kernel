/*
 * pic.h – x86 8259A Programmable Interrupt Controller
 */

#ifndef PIC_H
#define PIC_H

#include "kernel/types.h"

/* Core functions */
void pic_init(void);
void pic_enable_irq(u8 irq);
void pic_disable_irq(u8 irq);
void pic_send_eoi(u8 irq);

/* Status functions */
u16 pic_get_irr(void);
u16 pic_get_isr(void);
u16 pic_get_mask(void);
void pic_set_mask(u16 mask);

/* Control functions */
void pic_mask_all(void);
void pic_unmask_all(void);
void pic_disable(void);
void pic_set_priority(u8 irq);

/* Utility functions */
u8 pic_is_spurious_irq(u8 irq);
const char* pic_get_irq_name(u8 irq);

#endif
