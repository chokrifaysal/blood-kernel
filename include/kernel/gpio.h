/*
 * gpio.h - simple GPIO abstraction
 */

#ifndef _BLOOD_GPIO_H
#define _BLOOD_GPIO_H

typedef enum {
    PA0, PA1, PA2, PA3, PA4, PA5, PA6, PA7,
    PB0, PB1, PB2, PB3, PB4, PB5, PB6, PB7,
    PC13, PC14, PC15
} gpio_pin_t;

typedef enum {
    GPIO_IN  = 0,
    GPIO_OUT = 1
} gpio_mode_t;

void gpio_init(void);
void gpio_set_mode(gpio_pin_t pin, gpio_mode_t mode);
void gpio_set(gpio_pin_t pin);
void gpio_clear(gpio_pin_t pin);
void gpio_toggle(gpio_pin_t pin);
u8   gpio_read(gpio_pin_t pin);

#endif
