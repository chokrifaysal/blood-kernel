/*
 * timer.h - x86 PIT & ARM SysTick
 */

#ifndef _BLOOD_TIMER_H
#define _BLOOD_TIMER_H

void timer_init(void);
void timer_delay(u32 ms);
u32  timer_ticks(void);

#endif
