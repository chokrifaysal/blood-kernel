/*
 * ata.h – x86 ATA/IDE disk driver
 */

#ifndef ATA_H
#define ATA_H

#include "kernel/types.h"

typedef struct {
    u16 base;
    u16 ctrl;
    u8 drive;
    u8 present;
    u32 sectors;
    char model[41];
    char serial[21];
} ata_device_t;

/* Core functions */
void ata_init(void);

/* I/O functions */
u8 ata_read_sectors(u8 drive, u32 lba, u8 count, void* buffer);
u8 ata_write_sectors(u8 drive, u32 lba, u8 count, const void* buffer);

/* Device functions */
ata_device_t* ata_get_device(u8 drive);
u8 ata_get_device_count(void);

#endif
