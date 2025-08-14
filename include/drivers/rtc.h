/*
 * rtc.h – x86 Real-Time Clock and CMOS
 */

#ifndef RTC_H
#define RTC_H

#include "kernel/types.h"

typedef struct {
    u8 second;
    u8 minute;
    u8 hour;
    u8 day;
    u8 month;
    u16 year;
    u8 weekday;
} rtc_time_t;

/* Core functions */
void rtc_init(void);
void rtc_read_time(rtc_time_t* time);
void rtc_set_time(const rtc_time_t* time);
u32 rtc_get_timestamp(void);

/* Alarm functions */
void rtc_set_alarm(u8 hour, u8 minute, u8 second);
void rtc_disable_alarm(void);

/* Interrupt handler */
void rtc_irq_handler(void);

/* CMOS functions */
u16 cmos_get_base_memory(void);
u16 cmos_get_extended_memory(void);
u32 cmos_get_extended_memory2(void);
u8 cmos_get_floppy_type(void);
u8 cmos_get_disk_type(void);
u16 cmos_get_equipment(void);
void cmos_set_byte(u8 reg, u8 value);
u8 cmos_get_byte(u8 reg);

/* Callback functions (weak) */
void rtc_alarm_callback(void);
void rtc_periodic_callback(void);
void rtc_update_callback(void);

#endif
