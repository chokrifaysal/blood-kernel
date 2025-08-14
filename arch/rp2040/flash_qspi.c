/*
 * flash_qspi.c – RP2040 QSPI flash W25Q16 2MB
 */

#include "kernel/types.h"

#define XIP_SSI_BASE 0x18000000UL
#define PADS_QSPI_BASE 0x40020000UL
#define IO_QSPI_BASE 0x40018000UL

typedef volatile struct {
    u32 CTRLR0;
    u32 CTRLR1;
    u32 SSIENR;
    u32 MWCR;
    u32 SER;
    u32 BAUDR;
    u32 TXFTLR;
    u32 RXFTLR;
    u32 TXFLR;
    u32 RXFLR;
    u32 SR;
    u32 IMR;
    u32 ISR;
    u32 RISR;
    u32 TXOICR;
    u32 RXOICR;
    u32 RXUICR;
    u32 MSTICR;
    u32 ICR;
    u32 DMACR;
    u32 DMATDLR;
    u32 DMARDLR;
    u32 IDR;
    u32 SSI_VERSION_ID;
    u32 DR0;
    u32 _pad1[35];
    u32 RX_SAMPLE_DLY;
    u32 SPI_CTRLR0;
    u32 TXD_DRIVE_EDGE;
} XIP_SSI_TypeDef;

static XIP_SSI_TypeDef* const XIP_SSI = (XIP_SSI_TypeDef*)XIP_SSI_BASE;

/* Flash commands */
#define CMD_READ_STATUS    0x05
#define CMD_WRITE_ENABLE   0x06
#define CMD_WRITE_DISABLE  0x04
#define CMD_READ_DATA      0x03
#define CMD_FAST_READ      0x0B
#define CMD_PAGE_PROGRAM   0x02
#define CMD_SECTOR_ERASE   0x20
#define CMD_BLOCK_ERASE    0xD8
#define CMD_CHIP_ERASE     0xC7
#define CMD_READ_ID        0x9F
#define CMD_QUAD_READ      0xEB

static void flash_wait_ready(void) {
    while (XIP_SSI->SR & (1<<0)); /* Wait for not busy */
}

static void flash_cs_force(u8 enable) {
    if (enable) {
        *(u32*)(IO_QSPI_BASE + 0x0C) = 0x2; /* Force CS low */
    } else {
        *(u32*)(IO_QSPI_BASE + 0x0C) = 0x3; /* Release CS */
    }
}

static void flash_exit_xip(void) {
    /* Disable XIP mode */
    XIP_SSI->SSIENR = 0;
    
    /* Standard SPI mode */
    XIP_SSI->CTRLR0 = (7 << 16) | (0 << 8) | (0 << 6) | (0 << 4) | 0;
    XIP_SSI->BAUDR = 4; /* Slow for programming */
    
    /* Enable SSI */
    XIP_SSI->SSIENR = 1;
}

static void flash_enter_xip(void) {
    /* Disable SSI */
    XIP_SSI->SSIENR = 0;
    
    /* XIP mode configuration */
    XIP_SSI->CTRLR0 = (31 << 16) | (2 << 8) | (0 << 6) | (0 << 4) | 0;
    XIP_SSI->SPI_CTRLR0 = (CMD_QUAD_READ << 24) | (6 << 11) | (2 << 8) | (8 << 2) | 0;
    XIP_SSI->BAUDR = 2; /* Fast for execution */
    
    /* Enable SSI */
    XIP_SSI->SSIENR = 1;
}

static u8 flash_read_status(void) {
    flash_wait_ready();
    
    XIP_SSI->DR0 = CMD_READ_STATUS;
    XIP_SSI->DR0 = 0; /* Dummy byte */
    
    flash_wait_ready();
    
    /* Read response */
    u32 dummy = XIP_SSI->DR0;
    u8 status = XIP_SSI->DR0 & 0xFF;
    
    (void)dummy;
    return status;
}

static void flash_write_enable(void) {
    flash_wait_ready();
    XIP_SSI->DR0 = CMD_WRITE_ENABLE;
    flash_wait_ready();
}

static void flash_wait_write_done(void) {
    while (flash_read_status() & 1); /* Wait for WIP clear */
}

void flash_init(void) {
    /* Configure QSPI pads */
    *(u32*)(PADS_QSPI_BASE + 0x04) = 0x52; /* SCLK */
    *(u32*)(PADS_QSPI_BASE + 0x08) = 0x52; /* SD0 */
    *(u32*)(PADS_QSPI_BASE + 0x0C) = 0x52; /* SD1 */
    *(u32*)(PADS_QSPI_BASE + 0x10) = 0x52; /* SD2 */
    *(u32*)(PADS_QSPI_BASE + 0x14) = 0x52; /* SD3 */
    *(u32*)(PADS_QSPI_BASE + 0x18) = 0x52; /* SS */
    
    /* Configure QSPI GPIO */
    *(u32*)(IO_QSPI_BASE + 0x04) = 0; /* SCLK function */
    *(u32*)(IO_QSPI_BASE + 0x14) = 0; /* SD0 function */
    *(u32*)(IO_QSPI_BASE + 0x24) = 0; /* SD1 function */
    *(u32*)(IO_QSPI_BASE + 0x34) = 0; /* SD2 function */
    *(u32*)(IO_QSPI_BASE + 0x44) = 0; /* SD3 function */
    *(u32*)(IO_QSPI_BASE + 0x54) = 0; /* SS function */
}

u32 flash_read_id(void) {
    flash_exit_xip();
    flash_cs_force(1);
    
    XIP_SSI->DR0 = CMD_READ_ID;
    XIP_SSI->DR0 = 0; /* Dummy */
    XIP_SSI->DR0 = 0; /* Dummy */
    XIP_SSI->DR0 = 0; /* Dummy */
    
    flash_wait_ready();
    
    u32 dummy = XIP_SSI->DR0;
    u32 id = (XIP_SSI->DR0 << 16) | (XIP_SSI->DR0 << 8) | XIP_SSI->DR0;
    
    flash_cs_force(0);
    flash_enter_xip();
    
    (void)dummy;
    return id;
}

void flash_read(u32 addr, u8* buf, u32 len) {
    /* Use XIP mapping for reads */
    const u8* xip_base = (const u8*)0x10000000;
    for (u32 i = 0; i < len; i++) {
        buf[i] = xip_base[addr + i];
    }
}

void flash_sector_erase(u32 addr) {
    flash_exit_xip();
    flash_cs_force(1);
    
    flash_write_enable();
    
    XIP_SSI->DR0 = CMD_SECTOR_ERASE;
    XIP_SSI->DR0 = (addr >> 16) & 0xFF;
    XIP_SSI->DR0 = (addr >> 8) & 0xFF;
    XIP_SSI->DR0 = addr & 0xFF;
    
    flash_wait_ready();
    flash_cs_force(0);
    
    flash_wait_write_done();
    flash_enter_xip();
}

void flash_page_program(u32 addr, const u8* buf, u32 len) {
    if (len > 256) len = 256; /* Page size limit */
    
    flash_exit_xip();
    flash_cs_force(1);
    
    flash_write_enable();
    
    XIP_SSI->DR0 = CMD_PAGE_PROGRAM;
    XIP_SSI->DR0 = (addr >> 16) & 0xFF;
    XIP_SSI->DR0 = (addr >> 8) & 0xFF;
    XIP_SSI->DR0 = addr & 0xFF;
    
    for (u32 i = 0; i < len; i++) {
        XIP_SSI->DR0 = buf[i];
    }
    
    flash_wait_ready();
    flash_cs_force(0);
    
    flash_wait_write_done();
    flash_enter_xip();
}

void flash_write(u32 addr, const u8* buf, u32 len) {
    while (len > 0) {
        u32 page_offset = addr & 0xFF;
        u32 page_remaining = 256 - page_offset;
        u32 chunk_size = (len < page_remaining) ? len : page_remaining;
        
        flash_page_program(addr, buf, chunk_size);
        
        addr += chunk_size;
        buf += chunk_size;
        len -= chunk_size;
    }
}

void flash_chip_erase(void) {
    flash_exit_xip();
    flash_cs_force(1);
    
    flash_write_enable();
    
    XIP_SSI->DR0 = CMD_CHIP_ERASE;
    
    flash_wait_ready();
    flash_cs_force(0);
    
    flash_wait_write_done();
    flash_enter_xip();
}

/* Bootloader functions */
#define BOOTLOADER_MAGIC 0xB007C0DE

typedef struct {
    u32 magic;
    u32 size;
    u32 crc32;
    u32 entry_point;
} boot_header_t;

static u32 crc32_table[256];
static u8 crc32_init = 0;

static void crc32_generate_table(void) {
    for (u32 i = 0; i < 256; i++) {
        u32 crc = i;
        for (u8 j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
        crc32_table[i] = crc;
    }
    crc32_init = 1;
}

static u32 crc32_calc(const u8* data, u32 len) {
    if (!crc32_init) crc32_generate_table();
    
    u32 crc = 0xFFFFFFFF;
    for (u32 i = 0; i < len; i++) {
        crc = crc32_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFF;
}

u8 flash_boot_validate(u32 addr) {
    boot_header_t header;
    flash_read(addr, (u8*)&header, sizeof(header));
    
    if (header.magic != BOOTLOADER_MAGIC) return 0;
    if (header.size > 0x100000) return 0; /* 1MB max */
    
    /* Validate CRC */
    u8* image = (u8*)0x20000000; /* Use RAM for temp storage */
    flash_read(addr + sizeof(header), image, header.size);
    
    u32 calc_crc = crc32_calc(image, header.size);
    return (calc_crc == header.crc32);
}

void flash_boot_jump(u32 addr) {
    boot_header_t header;
    flash_read(addr, (u8*)&header, sizeof(header));
    
    if (header.magic == BOOTLOADER_MAGIC) {
        void (*app_entry)(void) = (void(*)(void))header.entry_point;
        app_entry();
    }
}
