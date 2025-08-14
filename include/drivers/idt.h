/*
 * idt.h – x86 Interrupt Descriptor Table
 */

#ifndef IDT_H
#define IDT_H

#include "kernel/types.h"

/* Core functions */
void idt_init(void);

/* Interrupt control */
void enable_interrupts(void);
void disable_interrupts(void);
u32 get_interrupt_flag(void);

/* Atomic operations */
void atomic_inc(volatile u32* ptr);
void atomic_dec(volatile u32* ptr);
void atomic_add(volatile u32* ptr, u32 value);
void atomic_sub(volatile u32* ptr, u32 value);
u32 atomic_xchg(volatile u32* ptr, u32 new_value);
u32 atomic_cmpxchg(volatile u32* ptr, u32 expected, u32 new_value);

/* Memory barriers */
void memory_barrier(void);
void read_barrier(void);
void write_barrier(void);

/* Handlers */
void isr_handler(u32 int_no, u32 err_code, u32 eip, u32 cs, u32 eflags);
void irq_handler(u32 irq_no);

#endif
