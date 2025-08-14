/*
 * adc0.c – AVR128DA48 ADC0 12-bit @ 1.25 MHz
 */

#include "kernel/types.h"
#include <avr/io.h>

void adc_init(void) {
    /* VREF = VDD, prescaler /16 → 20MHz/16 = 1.25 MHz */
    VREF.ADC0REF = 0;                         // VDD ref
    ADC0.CTRLC = (4<<0);                      // presc /16
    ADC0.CTRLA = (1<<0);                      // enable
}

u16 adc_read(u8 ch) {
    ADC0.MUXPOS = ch;                         // select channel
    ADC0.COMMAND = (1<<0);                    // start conv
    while (!(ADC0.INTFLAGS & (1<<0)));       // wait RESRDY
    ADC0.INTFLAGS = (1<<0);                   // clear flag
    return ADC0.RES;
}

/* Read internal temp sensor */
u16 adc_temp(void) {
    return adc_read(0x42);                    // TEMPSENSE
}

/* Read VDD/10 */
u16 adc_vdd(void) {
    return adc_read(0x1C);                    // VDDDIV10
}
