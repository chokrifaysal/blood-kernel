/*
 * pit.h – x86 8253/8254 Programmable Interval Timer
 */

#ifndef PIT_H
#define PIT_H

#include "kernel/types.h"

/* Core functions */
void pit_init(u32 frequency);
void pit_irq_handler(void);

/* Timing functions */
u32 pit_get_ticks(void);
u32 pit_get_frequency(void);
void pit_delay(u32 ms);
void pit_delay_us(u32 us);
u64 pit_get_uptime_ms(void);
u64 pit_get_uptime_us(void);

/* Sound functions */
void pit_beep(u32 frequency, u32 duration_ms);

/* Advanced functions */
void pit_reset(void);
void pit_calibrate_delay_loop(void);
void pit_set_channel1(u16 divisor);
u16 pit_read_counter(u8 channel);
void pit_oneshot(u8 channel, u16 count);

#endif
