/*
 * qspi_flash.h – QSPI flash driver (SST26VF032B)
 */

#ifndef _BLOOD_QSPI_FLASH_H
#define _BLOOD_QSPI_FLASH_H

#include "kernel/types.h"

void qspi_init(void);
void qspi_erase_sector(u32 addr);
void qspi_write_page(u32 addr, const u8* buf, u32 len);

#endif
