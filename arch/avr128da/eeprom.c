/*
 * eeprom.c – AVR128DA48 EEPROM 512 bytes
 */

#include "kernel/types.h"
#include <avr/io.h>

void eeprom_wait(void) {
    while (NVMCTRL.STATUS & (1<<0));         // wait EEBUSY
}

u8 eeprom_read(u16 addr) {
    eeprom_wait();
    return *(u8*)(0x1400 + addr);            // EEPROM base
}

void eeprom_write(u16 addr, u8 data) {
    eeprom_wait();

    /* unlock sequence */
    __asm__ volatile("cli");
    NVMCTRL.CTRLA = 0x13;                    // EEWR cmd
    *(u8*)(0x1400 + addr) = data;
    __asm__ volatile("sei");
}

void eeprom_erase_page(u16 addr) {
    eeprom_wait();

    __asm__ volatile("cli");
    NVMCTRL.CTRLA = 0x12;                    // EEER cmd
    *(u8*)(0x1400 + (addr & 0xFFC0)) = 0xFF; // page aligned
    __asm__ volatile("sei");
}
