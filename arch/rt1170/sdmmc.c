/*
 * sdmmc.c – SDMMC 4-bit @ 50 MHz
 */

#include "kernel/types.h"

#define SDMMC_BASE 0x400B8000UL

typedef volatile struct {
    u32 DSADDR;
    u32 BLKATTR;
    u32 CMDARG;
    u32 CMDRSP[4];
    u32 PRSSTAT;
    u32 PROCTL;
    u32 SYSCTL;
} SDMMC_TypeDef;

static SDMMC_TypeDef* const SDMMC = (SDMMC_TypeDef*)SDMMC_BASE;

void sdmmc_init(void) {
    SDMMC->SYSCTL = (1<<24) | (1<<0); // clock enable
    SDMMC->PROCTL = (1<<3);           // 4-bit bus
}

u8 sdmmc_read_block(u32 lba, u8* buf) {
    SDMMC->CMDARG = lba;
    SDMMC->BLKATTR = 512;
    /* polling read */
    for (u32 i = 0; i < 512; i += 4) {
        *(u32*)(buf + i) = SDMMC->CMDRSP[0];
    }
    return 0;
}
