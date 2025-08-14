
/*
 * rtc.c – x86 MC146818 Real-Time Clock and CMOS
 */

#include "kernel/types.h"

#define CMOS_ADDRESS 0x70
#define CMOS_DATA    0x71

/* CMOS registers */
#define RTC_SECONDS       0x00
#define RTC_MINUTES       0x02
#define RTC_HOURS         0x04
#define RTC_WEEKDAY       0x06
#define RTC_DAY           0x07
#define RTC_MONTH         0x08
#define RTC_YEAR          0x09
#define RTC_STATUS_A      0x0A
#define RTC_STATUS_B      0x0B
#define RTC_STATUS_C      0x0C
#define RTC_STATUS_D      0x0D

/* CMOS memory layout */
#define CMOS_FLOPPY_TYPE  0x10
#define CMOS_DISK_TYPE    0x12
#define CMOS_EQUIPMENT    0x14
#define CMOS_BASE_MEM_LO  0x15
#define CMOS_BASE_MEM_HI  0x16
#define CMOS_EXT_MEM_LO   0x17
#define CMOS_EXT_MEM_HI   0x18
#define CMOS_DISK_C       0x19
#define CMOS_DISK_D       0x1A
#define CMOS_EXT_MEM2_LO  0x30
#define CMOS_EXT_MEM2_HI  0x31
#define CMOS_CENTURY      0x32

/* Status register bits */
#define RTC_UIP           0x80  /* Update in progress */
#define RTC_24HOUR        0x02  /* 24-hour format */
#define RTC_BINARY        0x04  /* Binary mode */
#define RTC_SQWE          0x08  /* Square wave enable */
#define RTC_UIE           0x10  /* Update interrupt enable */
#define RTC_AIE           0x20  /* Alarm interrupt enable */
#define RTC_PIE           0x40  /* Periodic interrupt enable */

typedef struct {
    u8 second;
    u8 minute;
    u8 hour;
    u8 day;
    u8 month;
    u16 year;
    u8 weekday;
} rtc_time_t;

static u8 rtc_binary_mode = 0;
static u8 rtc_24hour_mode = 0;

static inline void outb(u16 port, u8 val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline u8 inb(u16 port) {
    u8 val;
    __asm__ volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static u8 cmos_read(u8 reg) {
    outb(CMOS_ADDRESS, reg);
    return inb(CMOS_DATA);
}

static void cmos_write(u8 reg, u8 value) {
    outb(CMOS_ADDRESS, reg);
    outb(CMOS_DATA, value);
}

static u8 bcd_to_binary(u8 bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

static u8 binary_to_bcd(u8 binary) {
    return ((binary / 10) << 4) | (binary % 10);
}

static void rtc_wait_update(void) {
    /* Wait for update cycle to complete */
    while (cmos_read(RTC_STATUS_A) & RTC_UIP);
}

void rtc_init(void) {
    /* Disable NMI and read status B */
    u8 status_b = cmos_read(RTC_STATUS_B);
    
    /* Check format modes */
    rtc_binary_mode = (status_b & RTC_BINARY) != 0;
    rtc_24hour_mode = (status_b & RTC_24HOUR) != 0;
    
    /* Enable 24-hour and binary mode */
    status_b |= RTC_24HOUR | RTC_BINARY;
    cmos_write(RTC_STATUS_B, status_b);
    
    rtc_binary_mode = 1;
    rtc_24hour_mode = 1;
    
    /* Clear any pending interrupts */
    cmos_read(RTC_STATUS_C);
}

void rtc_read_time(rtc_time_t* time) {
    rtc_wait_update();
    
    time->second = cmos_read(RTC_SECONDS);
    time->minute = cmos_read(RTC_MINUTES);
    time->hour = cmos_read(RTC_HOURS);
    time->day = cmos_read(RTC_DAY);
    time->month = cmos_read(RTC_MONTH);
    time->year = cmos_read(RTC_YEAR);
    time->weekday = cmos_read(RTC_WEEKDAY);
    
    /* Convert from BCD if necessary */
    if (!rtc_binary_mode) {
        time->second = bcd_to_binary(time->second);
        time->minute = bcd_to_binary(time->minute);
        time->hour = bcd_to_binary(time->hour);
        time->day = bcd_to_binary(time->day);
        time->month = bcd_to_binary(time->month);
        time->year = bcd_to_binary(time->year);
        time->weekday = bcd_to_binary(time->weekday);
    }
    
    /* Handle century */
    if (time->year < 80) {
        time->year += 2000;
    } else {
        time->year += 1900;
    }
    
    /* Try to read century register */
    u8 century = cmos_read(CMOS_CENTURY);
    if (century != 0 && century != 0xFF) {
        if (!rtc_binary_mode) {
            century = bcd_to_binary(century);
        }
        time->year = (century * 100) + (time->year % 100);
    }
}

void rtc_set_time(const rtc_time_t* time) {
    u8 second = time->second;
    u8 minute = time->minute;
    u8 hour = time->hour;
    u8 day = time->day;
    u8 month = time->month;
    u8 year = time->year % 100;
    u8 century = time->year / 100;
    
    /* Convert to BCD if necessary */
    if (!rtc_binary_mode) {
        second = binary_to_bcd(second);
        minute = binary_to_bcd(minute);
        hour = binary_to_bcd(hour);
        day = binary_to_bcd(day);
        month = binary_to_bcd(month);
        year = binary_to_bcd(year);
        century = binary_to_bcd(century);
    }
    
    /* Disable updates */
    u8 status_b = cmos_read(RTC_STATUS_B);
    cmos_write(RTC_STATUS_B, status_b | 0x80);
    
    rtc_wait_update();
    
    /* Set time */
    cmos_write(RTC_SECONDS, second);
    cmos_write(RTC_MINUTES, minute);
    cmos_write(RTC_HOURS, hour);
    cmos_write(RTC_DAY, day);
    cmos_write(RTC_MONTH, month);
    cmos_write(RTC_YEAR, year);
    cmos_write(CMOS_CENTURY, century);
    
    /* Re-enable updates */
    cmos_write(RTC_STATUS_B, status_b);
}

u32 rtc_get_timestamp(void) {
    rtc_time_t time;
    rtc_read_time(&time);
    
    /* Convert to Unix timestamp (seconds since 1970) */
    u32 days = 0;
    
    /* Count days from years */
    for (u16 y = 1970; y < time.year; y++) {
        if ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0)) {
            days += 366; /* Leap year */
        } else {
            days += 365;
        }
    }
    
    /* Days in months */
    u8 days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    /* Check for leap year */
    if (((time.year % 4 == 0 && time.year % 100 != 0) || (time.year % 400 == 0)) && time.month > 2) {
        days += 1;
    }
    
    /* Count days from months */
    for (u8 m = 1; m < time.month; m++) {
        days += days_in_month[m - 1];
    }
    
    /* Add days */
    days += time.day - 1;
    
    return days * 86400 + time.hour * 3600 + time.minute * 60 + time.second;
}

void rtc_set_alarm(u8 hour, u8 minute, u8 second) {
    if (!rtc_binary_mode) {
        hour = binary_to_bcd(hour);
        minute = binary_to_bcd(minute);
        second = binary_to_bcd(second);
    }
    
    /* Set alarm time */
    cmos_write(0x01, second);  /* Alarm seconds */
    cmos_write(0x03, minute);  /* Alarm minutes */
    cmos_write(0x05, hour);    /* Alarm hours */
    
    /* Enable alarm interrupt */
    u8 status_b = cmos_read(RTC_STATUS_B);
    cmos_write(RTC_STATUS_B, status_b | RTC_AIE);
}

void rtc_disable_alarm(void) {
    u8 status_b = cmos_read(RTC_STATUS_B);
    cmos_write(RTC_STATUS_B, status_b & ~RTC_AIE);
}

void rtc_irq_handler(void) {
    /* Read status C to clear interrupt */
    u8 status_c = cmos_read(RTC_STATUS_C);
    
    if (status_c & 0x20) {
        /* Alarm interrupt */
        extern void rtc_alarm_callback(void);
        rtc_alarm_callback();
    }
    
    if (status_c & 0x40) {
        /* Periodic interrupt */
        extern void rtc_periodic_callback(void);
        rtc_periodic_callback();
    }
    
    if (status_c & 0x10) {
        /* Update interrupt */
        extern void rtc_update_callback(void);
        rtc_update_callback();
    }
}

/* CMOS memory functions */
u16 cmos_get_base_memory(void) {
    u16 low = cmos_read(CMOS_BASE_MEM_LO);
    u16 high = cmos_read(CMOS_BASE_MEM_HI);
    return (high << 8) | low;
}

u16 cmos_get_extended_memory(void) {
    u16 low = cmos_read(CMOS_EXT_MEM_LO);
    u16 high = cmos_read(CMOS_EXT_MEM_HI);
    return (high << 8) | low;
}

u32 cmos_get_extended_memory2(void) {
    u16 low = cmos_read(CMOS_EXT_MEM2_LO);
    u16 high = cmos_read(CMOS_EXT_MEM2_HI);
    return ((u32)(high << 8) | low) * 64; /* In KB */
}

u8 cmos_get_floppy_type(void) {
    return cmos_read(CMOS_FLOPPY_TYPE);
}

u8 cmos_get_disk_type(void) {
    return cmos_read(CMOS_DISK_TYPE);
}

u16 cmos_get_equipment(void) {
    return cmos_read(CMOS_EQUIPMENT);
}

void cmos_set_byte(u8 reg, u8 value) {
    if (reg >= 0x0E && reg <= 0x7F) {
        cmos_write(reg, value);
    }
}

u8 cmos_get_byte(u8 reg) {
    if (reg >= 0x0E && reg <= 0x7F) {
        return cmos_read(reg);
    }
    return 0;
}

/* Weak callback functions */
__attribute__((weak)) void rtc_alarm_callback(void) {
    /* Default empty implementation */
}

__attribute__((weak)) void rtc_periodic_callback(void) {
    /* Default empty implementation */
}

__attribute__((weak)) void rtc_update_callback(void) {
    /* Default empty implementation */
}
