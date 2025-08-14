/*
 * timer_hw.c – RP2040 hardware timer @ 1 MHz → 1 kHz tick
 */

#include "kernel/types.h"

#define TIMER_BASE 0x40054000UL
#define RESETS_BASE 0x4000C000UL

typedef volatile struct {
    u32 TIMEHW;
    u32 TIMELW;
    u32 TIMEHR;
    u32 TIMELR;
    u32 ALARM0;
    u32 ALARM1;
    u32 ALARM2;
    u32 ALARM3;
    u32 ARMED;
    u32 TIMERAWH;
    u32 TIMERAWL;
    u32 DBGPAUSE;
    u32 PAUSE;
    u32 INTR;
    u32 INTE;
    u32 INTF;
    u32 INTS;
} TIMER_TypeDef;

static TIMER_TypeDef* const TIMER = (TIMER_TypeDef*)TIMER_BASE;
static volatile u32 sys_ticks = 0;

void timer_init(void) {
    /* Reset timer */
    *(u32*)(RESETS_BASE + 0x0) &= ~(1<<21);
    while (*(u32*)(RESETS_BASE + 0x8) & (1<<21));
    *(u32*)(RESETS_BASE + 0x4) |= (1<<21);
    while (!(*(u32*)(RESETS_BASE + 0x8) & (1<<21)));
    
    /* Set alarm 0 for 1 kHz tick (1000 us) */
    TIMER->ALARM0 = TIMER->TIMERAWL + 1000;
    TIMER->INTE |= (1<<0);
    TIMER->ARMED |= (1<<0);
}

u32 timer_ticks(void) {
    u32 ticks;
    __asm__ volatile("cpsid i");
    ticks = sys_ticks;
    __asm__ volatile("cpsie i");
    return ticks;
}

void timer_delay(u32 ms) {
    u32 end = timer_ticks() + ms;
    while (timer_ticks() < end);
}

u64 timer_us(void) {
    u32 hi = TIMER->TIMEHR;
    u32 lo = TIMER->TIMELR;
    return ((u64)hi << 32) | lo;
}

void timer_delay_us(u32 us) {
    u64 end = timer_us() + us;
    while (timer_us() < end);
}

void timer_irq_handler(void) {
    if (TIMER->INTS & (1<<0)) {
        sys_ticks++;
        TIMER->INTR = (1<<0);
        TIMER->ALARM0 = TIMER->TIMERAWL + 1000;
        TIMER->ARMED |= (1<<0);
    }
}

/* Watchdog timer */
#define WATCHDOG_BASE 0x40058000UL

typedef volatile struct {
    u32 CTRL;
    u32 LOAD;
    u32 REASON;
    u32 SCRATCH[8];
    u32 TICK;
} WATCHDOG_TypeDef;

static WATCHDOG_TypeDef* const WATCHDOG = (WATCHDOG_TypeDef*)WATCHDOG_BASE;

void watchdog_init(u32 timeout_ms) {
    /* Set tick to 1 MHz (1 us per tick) */
    WATCHDOG->TICK = 0x40000000 | 12; /* 12 MHz / 12 = 1 MHz */
    
    /* Load timeout value */
    WATCHDOG->LOAD = timeout_ms * 1000; /* Convert ms to us */
    
    /* Enable watchdog */
    WATCHDOG->CTRL = 0x40000000;
}

void watchdog_feed(void) {
    WATCHDOG->LOAD = WATCHDOG->LOAD;
}

u32 watchdog_get_reason(void) {
    return WATCHDOG->REASON;
}

/* RTC */
#define RTC_BASE 0x4005C000UL

typedef volatile struct {
    u32 CLKDIV_M1;
    u32 SETUP_0;
    u32 SETUP_1;
    u32 CTRL;
    u32 IRQ_SETUP_0;
    u32 IRQ_SETUP_1;
    u32 RTC_1;
    u32 RTC_0;
    u32 INTR;
    u32 INTE;
    u32 INTF;
    u32 INTS;
} RTC_TypeDef;

static RTC_TypeDef* const RTC = (RTC_TypeDef*)RTC_BASE;

void rtc_init(void) {
    /* Reset RTC */
    *(u32*)(RESETS_BASE + 0x0) &= ~(1<<18);
    while (*(u32*)(RESETS_BASE + 0x8) & (1<<18));
    *(u32*)(RESETS_BASE + 0x4) |= (1<<18);
    while (!(*(u32*)(RESETS_BASE + 0x8) & (1<<18)));
    
    /* Set clock divider for 32.768 kHz crystal */
    RTC->CLKDIV_M1 = 32768 - 1;
    
    /* Setup RTC */
    RTC->SETUP_0 = (2025 << 12) | (8 << 8) | 14; /* Year 2025, Month 8, Day 14 */
    RTC->SETUP_1 = (16 << 16) | (30 << 8) | 0;   /* Hour 16, Min 30, Sec 0 */
    
    /* Load and start */
    RTC->CTRL = (1<<4) | (1<<0); /* LOAD | RTC_ENABLE */
    while (RTC->CTRL & (1<<4));  /* Wait for LOAD to clear */
}

u32 rtc_get_time(void) {
    return RTC->RTC_0;
}

u32 rtc_get_date(void) {
    return RTC->RTC_1;
}

void rtc_set_alarm(u32 time, u32 date) {
    RTC->IRQ_SETUP_0 = time;
    RTC->IRQ_SETUP_1 = date;
    RTC->INTE |= (1<<0);
}

/* PWM */
#define PWM_BASE 0x40050000UL

typedef volatile struct {
    u32 CSR;
    u32 DIV;
    u32 CTR;
    u32 CC;
    u32 TOP;
} PWM_SLICE_TypeDef;

static PWM_SLICE_TypeDef* const PWM = (PWM_SLICE_TypeDef*)PWM_BASE;

void pwm_init(u8 slice, u8 chan, u16 freq_hz) {
    /* Reset PWM */
    *(u32*)(RESETS_BASE + 0x0) &= ~(1<<14);
    while (*(u32*)(RESETS_BASE + 0x8) & (1<<14));
    *(u32*)(RESETS_BASE + 0x4) |= (1<<14);
    while (!(*(u32*)(RESETS_BASE + 0x8) & (1<<14)));
    
    PWM_SLICE_TypeDef* slice_reg = &PWM[slice];
    
    /* Calculate divider: 125 MHz / (freq_hz * 65536) */
    u32 div = (125000000UL << 4) / (freq_hz * 65536);
    slice_reg->DIV = div;
    slice_reg->TOP = 65535;
    slice_reg->CSR = (1<<0); /* Enable */
}

void pwm_set_duty(u8 slice, u8 chan, u16 duty) {
    PWM_SLICE_TypeDef* slice_reg = &PWM[slice];
    if (chan == 0) {
        slice_reg->CC = (slice_reg->CC & 0xFFFF0000) | duty;
    } else {
        slice_reg->CC = (slice_reg->CC & 0x0000FFFF) | (duty << 16);
    }
}

/* ADC */
#define ADC_BASE 0x4004C000UL

typedef volatile struct {
    u32 CS;
    u32 RESULT;
    u32 FCS;
    u32 FIFO;
    u32 DIV;
    u32 INTR;
    u32 INTE;
    u32 INTF;
    u32 INTS;
} ADC_TypeDef;

static ADC_TypeDef* const ADC = (ADC_TypeDef*)ADC_BASE;

void adc_init(void) {
    /* Reset ADC */
    *(u32*)(RESETS_BASE + 0x0) &= ~(1<<0);
    while (*(u32*)(RESETS_BASE + 0x8) & (1<<0));
    *(u32*)(RESETS_BASE + 0x4) |= (1<<0);
    while (!(*(u32*)(RESETS_BASE + 0x8) & (1<<0)));
    
    /* Enable ADC */
    ADC->CS = (1<<0);
}

u16 adc_read(u8 channel) {
    if (channel > 4) return 0;
    
    /* Select channel and start conversion */
    ADC->CS = (1<<0) | (channel << 12) | (1<<2);
    
    /* Wait for conversion */
    while (!(ADC->CS & (1<<8)));
    
    return ADC->RESULT & 0xFFF;
}

u16 adc_read_temp(void) {
    /* Enable temp sensor */
    ADC->CS |= (1<<1);
    return adc_read(4);
}
