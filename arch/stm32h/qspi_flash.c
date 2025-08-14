/*
 * qspi_flash.c – STM32H745 QUADSPI W25Q128
 */

#include "kernel/types.h"

#define QUADSPI_BASE 0x52005000UL
#define RCC_AHB3ENR (*(volatile u32*)0x580244D4UL)

typedef volatile struct {
    u32 CR;
    u32 DCR;
    u32 SR;
    u32 FCR;
    u32 DLR;
    u32 CCR;
    u32 AR;
    u32 ABR;
    u32 DR;
    u32 PSMKR;
    u32 PSMAR;
    u32 PIR;
    u32 LPTR;
} QUADSPI_TypeDef;

static QUADSPI_TypeDef* const QSPI = (QUADSPI_TypeDef*)QUADSPI_BASE;

/* Flash commands */
#define CMD_READ_ID     0x9F
#define CMD_READ_DATA   0x03
#define CMD_FAST_READ   0x0B
#define CMD_QUAD_READ   0xEB
#define CMD_WRITE_EN    0x06
#define CMD_WRITE_DIS   0x04
#define CMD_PAGE_PROG   0x02
#define CMD_SECTOR_ERASE 0x20
#define CMD_BLOCK_ERASE  0xD8
#define CMD_CHIP_ERASE   0xC7
#define CMD_READ_STATUS  0x05

static void qspi_wait_ready(void) {
    while (QSPI->SR & (1<<5)); // BUSY
}

static void qspi_cmd(u8 cmd, u32 addr, u8* data, u16 len, u8 mode) {
    qspi_wait_ready();
    
    /* Data length */
    QSPI->DLR = len ? len - 1 : 0;
    
    /* Command configuration */
    u32 ccr = cmd | (mode << 8);
    if (addr != 0xFFFFFFFF) {
        ccr |= (1 << 10) | (2 << 12); // ADMODE=1, ADSIZE=24-bit
        QSPI->AR = addr;
    }
    if (len > 0) {
        ccr |= (1 << 24); // DMODE=1
    }
    
    QSPI->CCR = ccr;
    
    /* Transfer data */
    if (mode == 0 && len > 0) { // Write
        for (u16 i = 0; i < len; i++) {
            while (!(QSPI->SR & (1<<2))); // FTF
            *(volatile u8*)&QSPI->DR = data[i];
        }
    } else if (mode == 1 && len > 0) { // Read
        for (u16 i = 0; i < len; i++) {
            while (!(QSPI->SR & (1<<2))); // FTF
            data[i] = *(volatile u8*)&QSPI->DR;
        }
    }
    
    qspi_wait_ready();
}

void qspi_init(void) {
    /* Enable QSPI clock */
    RCC_AHB3ENR |= (1<<14);
    
    /* Reset QSPI */
    QSPI->CR = 0;
    
    /* Configure: prescaler=1, FIFO threshold=1 */
    QSPI->CR = (1 << 24) | (0 << 8) | (1 << 0);
    
    /* Device configuration: 16MB flash, CS high time=1 cycle */
    QSPI->DCR = (23 << 16) | (0 << 8);
    
    /* Enable QSPI */
    QSPI->CR |= (1 << 0);
}

u32 qspi_read_id(void) {
    u8 id[3];
    qspi_cmd(CMD_READ_ID, 0xFFFFFFFF, id, 3, 1);
    return (id[0] << 16) | (id[1] << 8) | id[2];
}

void qspi_read(u32 addr, u8* buf, u16 len) {
    qspi_cmd(CMD_FAST_READ, addr, buf, len, 1);
}

void qspi_write_enable(void) {
    qspi_cmd(CMD_WRITE_EN, 0xFFFFFFFF, 0, 0, 0);
}

u8 qspi_read_status(void) {
    u8 status;
    qspi_cmd(CMD_READ_STATUS, 0xFFFFFFFF, &status, 1, 1);
    return status;
}

void qspi_wait_write_done(void) {
    while (qspi_read_status() & 1); // WIP
}

void qspi_page_program(u32 addr, const u8* buf, u16 len) {
    qspi_write_enable();
    qspi_cmd(CMD_PAGE_PROG, addr, (u8*)buf, len, 0);
    qspi_wait_write_done();
}

void qspi_sector_erase(u32 addr) {
    qspi_write_enable();
    qspi_cmd(CMD_SECTOR_ERASE, addr, 0, 0, 0);
    qspi_wait_write_done();
}

void qspi_write(u32 addr, const u8* buf, u16 len) {
    while (len > 0) {
        u16 page_rem = 256 - (addr & 0xFF);
        u16 chunk = (len < page_rem) ? len : page_rem;
        
        qspi_page_program(addr, buf, chunk);
        
        addr += chunk;
        buf += chunk;
        len -= chunk;
    }
}
