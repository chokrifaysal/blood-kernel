/*
 * rtc_slow.c – 8 kB RTC slow memory for persistent vars
 */

#include "kernel/types.h"

#define RTC_SLOW_BASE 0x50000000UL

void rtc_store(u32 offset, u32 val) {
    *(volatile u32*)(RTC_SLOW_BASE + offset) = val;
}

u32 rtc_load(u32 offset) {
    return *(volatile u32*)(RTC_SLOW_BASE + offset);
}
