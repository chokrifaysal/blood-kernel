/*
 * sd.c - SPI-mode SD driver, raw blocks only
 */

#include "kernel/sd.h"
#include "kernel/spi.h"
#include "kernel/types.h"
#include "uart.h"

static u8 sd_cmd(u8 cmd, u32 arg) {
    u8 crc = 0xFF;
    spi_xfer(0xFF);          // dummy
    spi_xfer(cmd | 0x40);
    spi_xfer(arg >> 24);
    spi_xfer(arg >> 16);
    spi_xfer(arg >> 8);
    spi_xfer(arg);
    spi_xfer(crc);
    
    for (int i = 0; i < 8; i++) {
        u8 r = spi_xfer(0xFF);
        if ((r & 0x80) == 0) return r;
    }
    return 0xFF;
}

void sd_init(void) {
    spi_init(SPI_MODE0);
    spi_cs_dis(4);
    
    // 80 clocks idle
    for (int i = 0; i < 10; i++) spi_xfer(0xFF);
    
    spi_cs_en(4);
    if (sd_cmd(0, 0) != 0x01) {           // GO_IDLE
        uart_puts("SD init fail\r\n");
        return;
    }
    
    // wait ready
    while (sd_cmd(1, 0) != 0);
    
    spi_cs_dis(4);
    uart_puts("SD ready\r\n");
}

u8 sd_read(u32 block, u8* buf) {
    spi_cs_en(4);
    if (sd_cmd(17, block << 9) != 0) {    // READ_SINGLE
        spi_cs_dis(4);
        return 1;
    }
    
    // wait data token
    while (spi_xfer(0xFF) != 0xFE);
    
    for (int i = 0; i < SD_BLOCK_SIZE; i++) {
        buf[i] = spi_xfer(0xFF);
    }
    spi_xfer(0xFF); spi_xfer(0xFF);   // CRC discard
    
    spi_cs_dis(4);
    return 0;
}

u8 sd_write(u32 block, const u8* buf) {
    spi_cs_en(4);
    if (sd_cmd(24, block << 9) != 0) {    // WRITE_SINGLE
        spi_cs_dis(4);
        return 1;
    }
    
    spi_xfer(0xFF);
    spi_xfer(0xFE);   // token
    
    for (int i = 0; i < SD_BLOCK_SIZE; i++) {
        spi_xfer(buf[i]);
    }
    spi_xfer(0xFF); spi_xfer(0xFF);   // dummy CRC
    
    u8 resp = spi_xfer(0xFF);
    if ((resp & 0x1F) != 0x05) {
        spi_cs_dis(4);
        return 1;
    }
    
    while (spi_xfer(0xFF) == 0);      // busy
    spi_cs_dis(4);
    return 0;
}

u32 sd_capacity(void) {
    return 0;   // TODO: read CSD
}
