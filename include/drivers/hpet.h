/*
 * hpet.h – x86 High Precision Event Timer
 */

#ifndef HPET_H
#define HPET_H

#include "kernel/types.h"

/* Core functions */
u8 hpet_init(u64 base_address);
u8 hpet_is_initialized(void);

/* Counter functions */
u64 hpet_get_counter(void);
u64 hpet_get_frequency(void);

/* Timing functions */
void hpet_udelay(u32 microseconds);
void hpet_ndelay(u32 nanoseconds);
u64 hpet_get_timestamp_ns(void);
u64 hpet_get_timestamp_us(void);

/* Timer functions */
u8 hpet_setup_timer(u8 timer_num, u32 period_us, u8 periodic, u8 irq);
void hpet_disable_timer(u8 timer_num);

/* Information functions */
u8 hpet_get_num_timers(void);
u8 hpet_is_legacy_capable(void);

/* Calibration */
void hpet_calibrate_tsc(u64* tsc_frequency);

/* Interrupt handler */
void hpet_irq_handler(u8 timer_num);

#endif
