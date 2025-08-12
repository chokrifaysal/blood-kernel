/*
 * spi.h - bare-metal SPI master for STM32
 */

#ifndef _BLOOD_SPI_H
#define _BLOOD_SPI_H

#include "kernel/types.h"

typedef enum {
    SPI_MODE0 = 0,  // CPOL=0 CPHA=0
    SPI_MODE1 = 1,  // CPOL=0 CPHA=1
    SPI_MODE2 = 2,  // CPOL=1 CPHA=0
    SPI_MODE3 = 3   // CPOL=1 CPHA=1
} spi_mode_t;

void spi_init(spi_mode_t mode);
u8   spi_xfer(u8 out);
void spi_cs_en(u8 cs);
void spi_cs_dis(u8 cs);

#endif
