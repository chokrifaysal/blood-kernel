/*
 * twi_slave.c – AVR128DA48 TWI0 slave @ 100 kHz
 */

#include "kernel/types.h"
#include <avr/io.h>

void twi_init(u8 addr) {
    TWI0.SADDR = addr << 1;
    TWI0.SCTRLA = (1<<ENABLE0) | (1<<DIEN0);
}

u8 twi_get_byte(void) {
    while (!(TWI0.SSTATUS & (1<<DIF0)));
    return TWI0.SDATA;
}
