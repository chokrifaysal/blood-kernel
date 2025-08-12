/*
 * flash.c - STM32F4 flash driver
 */

#include "kernel/flash.h"
#include "kernel/types.h"

#define FLASH_KEYR  (*(volatile u32*)0x40023C04)
#define FLASH_SR    (*(volatile u32*)0x40023C0C)
#define FLASH_CR    (*(volatile u32*)0x40023C10)

static void flash_unlock(void) {
    FLASH_KEYR = 0x45670123;
    FLASH_KEYR = 0xCDEF89AB;
}

static void flash_lock(void) {
    FLASH_CR |= (1<<31);
}

static void flash_wait(void) {
    while (FLASH_SR & (1<<16));
}

void flash_erase(u32 addr) {
    flash_unlock();
    FLASH_CR |= (1<<1);          // SER
    FLASH_CR |= ((addr - 0x08000000) >> 11) << 3;
    FLASH_CR |= (1<<16);         // STRT
    flash_wait();
    FLASH_CR &= ~(1<<1);
    flash_lock();
}

void flash_write(u32 addr, const u8* data, u32 len) {
    flash_unlock();
    FLASH_CR |= (1<<0);          // PG
    for (u32 i = 0; i < len; i += 4) {
        *(volatile u32*)(addr + i) = *(u32*)(data + i);
        flash_wait();
    }
    FLASH_CR &= ~(1<<0);
    flash_lock();
}
