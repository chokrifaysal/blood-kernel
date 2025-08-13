/*
 * eth_mac.c – RA6M5 Ethernet MAC + RMII PHY
 * 100 Mbit/s, DMA disabled, polling
 */

#include "kernel/types.h"

#define ETHERC_BASE 0x40070000UL
#define PHY_ADDR    0x01

typedef volatile struct {
    u32 ECMR;
    u32 RFLR;
    u32 TFCR;
    u32 TCR;
    u32 RCR;
    u32 ECSR;
    u32 ECSIPR;
    u32 PIR;
    u32 PSR;
    u32 RDMLR;
    u32 TFDR;
    u32 RFDR;
} ETHERC_TypeDef;

static ETHERC_TypeDef* const ETHERC = (ETHERC_TypeDef*)ETHERC_BASE;

void eth_init(void) {
    /* 200 MHz PCLKA → 100 Mbit RMII */
    ETHERC->ECMR = (1<<0);   // ECMR.RE
    /* PHY reset via GPIO */
}

void eth_tx(const u8* pkt, u16 len) {
    /* polling TX FIFO */
    for (u16 i = 0; i < len; i += 4) {
        ETHERC->TFDR = *(u32*)(pkt + i);
    }
}
