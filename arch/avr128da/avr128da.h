/*
 * avr128da.h – AVR128DA48 peripheral functions
 */

#ifndef AVR128DA_H
#define AVR128DA_H

#include "kernel/types.h"

/* TCA0 Timer */
void timer_init(void);
u32 timer_ticks(void);
void timer_delay(u32 ms);
void pwm_init(void);
void pwm_set(u8 duty);

/* ADC0 */
void adc_init(void);
u16 adc_read(u8 ch);
u16 adc_temp(void);
u16 adc_vdd(void);

/* EEPROM */
u8 eeprom_read(u16 addr);
void eeprom_write(u16 addr, u8 data);
void eeprom_erase_page(u16 addr);

/* GPIO (already in gpio_5v.c) */
void gpio_set_dir(u8 pin, u8 out);
void gpio_put(u8 pin, u8 val);
void gpio_toggle(u8 pin);

/* SPI (already in spi_master.c) */
void spi_init(void);
u8 spi_xfer(u8 out);

/* TWI (already in twi_slave.c) */
void twi_init(u8 addr);
u8 twi_get_byte(void);

#endif
