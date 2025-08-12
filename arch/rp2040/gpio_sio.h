/*
 * gpio_sio.h - RP2040 SIO GPIO wrapper
 */

#ifndef _BLOOD_GPIO_SIO_H
#define _BLOOD_GPIO_SIO_H

#include "kernel/types.h"

void gpio_set_dir(u8 pin, u8 out);
void gpio_put(u8 pin, u8 val);
void gpio_toggle(u8 pin);

#endif
