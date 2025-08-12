/*
 * spi.c - SPI1 master @ 42 MHz APB2 -> 21 MHz SCK
 */

#include "kernel/spi.h"
#include "kernel/types.h"

#define RCC_APB2ENR (*(volatile u32*)0x40023844)
#define GPIOA_BASE  0x40020000
#define SPI1_BASE   0x40013000

typedef volatile struct {
    u32 CR1; u32 CR2; u32 SR; u32 DR; u32 CRCPR; u32 RXCRCR; u32 TXCRCR; u32 I2SCFGR; u32 I2SPR;
} SPI_TypeDef;

static SPI_TypeDef* const SPI1 = (SPI_TypeDef*)SPI1_BASE;

void spi_init(spi_mode_t mode) {
    // clocks
    RCC_APB2ENR |= (1<<12);   // SPI1
    RCC_APB2ENR |= (1<<0);    // GPIOA
    
    // PA5=SCK, PA6=MISO, PA7=MOSI alt-fn
    *(volatile u32*)(GPIOA_BASE + 0x00) |= (2<<10) | (2<<12) | (2<<14);
    *(volatile u32*)(GPIOA_BASE + 0x20) |= (5<<20) | (5<<24) | (5<<28);
    
    // CR1: MSTR=1, BR=0b000 (/2), CPOL/CPHA from mode
    SPI1->CR1 = (1<<2) | (1<<6) | (mode<<0);
    SPI1->CR1 |= (1<<6);   // enable
}

u8 spi_xfer(u8 out) {
    while (!(SPI1->SR & (1<<1)));   // TXE
    *(volatile u8*)&SPI1->DR = out;
    while (!(SPI1->SR & (1<<0)));   // RXNE
    return *(volatile u8*)&SPI1->DR;
}

void spi_cs_en(u8 cs) {
    *(volatile u32*)(GPIOA_BASE + 0x18) |= (1<<cs);   // PA4 = CS0
}

void spi_cs_dis(u8 cs) {
    *(volatile u32*)(GPIOA_BASE + 0x18) |= (1<<(cs+16));   // reset
}
