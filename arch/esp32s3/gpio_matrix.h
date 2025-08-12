/*
 * gpio_matrix.h – ESP32-S3 GPIO matrix helpers
 */

#ifndef _BLOOD_GPIO_MATRIX_H
#define _BLOOD_GPIO_MATRIX_H

#include "kernel/types.h"

void gpio_set_dir(u8 pin, u8 out);
void gpio_set_level(u8 pin, u8 val);
void gpio_toggle(u8 pin);

#endif
