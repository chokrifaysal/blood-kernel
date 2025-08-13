/*
 * qspi.c – QSPI 50 MHz 4-line, sector erase + page program
 */

#include "kernel/types.h"

#define QSPI_BASE 0xA0000000UL
#define QUADSPI_CR  (*(volatile u32*)(QSPI_BASE + 0x00))
#define QUADSPI_DLR (*(volatile u32*)(QSPI_BASE + 0x0C))
#define QUADSPI_CCR (*(volatile u32*)(QSPI_BASE + 0x10))
#define QUADSPI_DR  (*(volatile u32*)(QSPI_BASE + 0x20))

void qspi_init(void) {
    QUADSPI_CR = (1<<0) | (50<<8); // EN + 50 MHz
}

void qspi_erase_sector(u32 addr) {
    QUADSPI_CCR = (0x20<<24) | (addr & 0x00FFFFFF);
}

void qspi_write_page(u32 addr, const u8* buf, u32 len) {
    QUADSPI_DLR = len - 1;
    for (u32 i = 0; i < len; i += 4) {
        QUADSPI_DR = *(u32*)(buf + i);
    }
}
