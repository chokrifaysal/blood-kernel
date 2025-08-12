/*
 * flash.h - minimal flash erase/write for bootloader
 */

#ifndef _BLOOD_FLASH_H
#define _BLOOD_FLASH_H

void flash_erase(u32 addr);
void flash_write(u32 addr, const u8* data, u32 len);

#endif
