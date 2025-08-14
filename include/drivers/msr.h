/*
 * msr.h – x86 Model Specific Registers
 */

#ifndef MSR_H
#define MSR_H

#include "kernel/types.h"

/* Core functions */
void msr_init(void);
u8 msr_is_supported(void);
u64 msr_read(u32 msr);
void msr_write(u32 msr, u64 value);

/* TSC functions */
u64 msr_get_tsc(void);
u64 msr_get_tsc_frequency(void);

/* APIC functions */
void msr_enable_apic(u64 apic_base);
u64 msr_get_apic_base(void);

/* Memory type functions */
void msr_set_pat(u64 pat_value);
u64 msr_get_pat(void);
void msr_setup_mtrr(u8 reg, u64 base, u64 mask, u8 type);
void msr_enable_mtrr(void);

/* System call functions */
void msr_enable_syscall(u64 star, u64 lstar, u64 cstar, u64 mask);

/* Segment base functions */
void msr_set_fs_base(u64 base);
void msr_set_gs_base(u64 base);
void msr_set_kernel_gs_base(u64 base);
u64 msr_get_fs_base(void);
u64 msr_get_gs_base(void);
u64 msr_get_kernel_gs_base(void);

/* Performance monitoring */
void msr_enable_performance_monitoring(void);
void msr_setup_performance_counter(u8 counter, u64 event_select);
u64 msr_read_performance_counter(u8 counter);

/* Microcode */
u32 msr_get_microcode_version(void);

#endif
