/*
 * pio_driver.h – RP2040 PIO state machines
 */

#ifndef _BLOOD_PIO_DRIVER_H
#define _BLOOD_PIO_DRIVER_H

#include "kernel/types.h"

typedef struct {
    u32 clkdiv;
    u8 wrap_top;
    u8 wrap_bottom;
    u8 autopull;
    u8 autopush;
    u8 pull_thresh;
    u8 push_thresh;
    u8 set_count;
    u8 out_count;
    u8 in_base;
    u8 set_base;
    u8 out_base;
} pio_config_t;

/* Core PIO functions */
void pio_init(void);
u8 pio_load_program(u8 pio_num, const u16* prog, u8 len);
void pio_sm_init(u8 pio_num, u8 sm, u8 offset, const pio_config_t* config);
void pio_sm_start(u8 pio_num, u8 sm);
void pio_sm_stop(u8 pio_num, u8 sm);
void pio_sm_put(u8 pio_num, u8 sm, u32 data);
u32 pio_sm_get(u8 pio_num, u8 sm);
u8 pio_sm_tx_full(u8 pio_num, u8 sm);
u8 pio_sm_rx_empty(u8 pio_num, u8 sm);

/* High-level functions */
void pio_load_blink(u8 pin);
void pio_uart_tx_init(u8 pin, u32 baud);
void pio_spi_init(u8 clk_pin, u8 mosi_pin, u8 miso_pin);
void pio_ws2812_init(u8 pin);

/* Timer functions */
void timer_init(void);
u32 timer_ticks(void);
void timer_delay(u32 ms);
u64 timer_us(void);
void timer_delay_us(u32 us);

/* Watchdog */
void watchdog_init(u32 timeout_ms);
void watchdog_feed(void);
u32 watchdog_get_reason(void);

/* RTC */
void rtc_init(void);
u32 rtc_get_time(void);
u32 rtc_get_date(void);
void rtc_set_alarm(u32 time, u32 date);

/* PWM */
void pwm_init(u8 slice, u8 chan, u16 freq_hz);
void pwm_set_duty(u8 slice, u8 chan, u16 duty);

/* ADC */
void adc_init(void);
u16 adc_read(u8 channel);
u16 adc_read_temp(void);

#endif
