/*
 * qspi_flash.c – 4-line QSPI, 50 MHz SCK
 */

#include "qspi_flash.h"
#include "kernel/types.h"

#define QSPI_BASE 0xBF8E0000UL

static volatile u32* const QSPI = (u32*)QSPI_BASE;

void qspi_init(void) {
    QSPI[0] = (1<<7) | (1<<15); // ON | QEN
}

void qspi_erase_sector(u32 addr) {
    QSPI[1] = 0x20;                 // erase command
    QSPI[2] = addr;
    while (QSPI[3] & (1<<0));       // busy
}

void qspi_write_page(u32 addr, const u8* buf, u32 len) {
    QSPI[1] = 0x02;                 // page program
    QSPI[2] = addr;
    for (u32 i = 0; i < len; i += 4) {
        QSPI[4] = *(u32*)(buf + i);
    }
    while (QSPI[3] & (1<<0));
}
