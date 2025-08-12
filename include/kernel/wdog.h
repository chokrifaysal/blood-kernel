/*
 * wdog.h - independent watchdog for automotive safety
 */

#ifndef _BLOOD_WDOG_H
#define _BLOOD_WDOG_H

void wdog_init(u32 timeout_ms);
void wdog_refresh(void);
void wdog_force_reset(void);

#endif
