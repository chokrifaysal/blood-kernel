/*
 * spi_master.c – AVR128DA48 SPI0 master @ 10 MHz
 */

#include "kernel/types.h"
#include <avr/io.h>

void spi_init(void) {
    PORTA.DIRSET = (1<<4);   // MOSI
    PORTA.DIRSET = (1<<6);   // SCK
    PORTA.DIRCLR = (1<<5);   // MISO
    SPI0.CTRLA = (1<<MSTR0) | (1<<ENABLE0);
    SPI0.CTRLB = 0;          // 8-bit
}

u8 spi_xfer(u8 out) {
    SPI0.DATA = out;
    while (!(SPI0.INTFLAGS & (1<<SPIF0)));
    return SPI0.DATA;
}
