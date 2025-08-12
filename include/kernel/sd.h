/*
 * sd.h - SD-card raw block layer
 */

#ifndef _BLOOD_SD_H
#define _BLOOD_SD_H

#include "kernel/types.h"

#define SD_BLOCK_SIZE 512

void sd_init(void);
u8   sd_read(u32 block, u8* buf);
u8   sd_write(u32 block, const u8* buf);
u32  sd_capacity(void);

#endif
