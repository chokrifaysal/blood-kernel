/*
 * ata.c – x86 ATA/IDE disk driver
 */

#include "kernel/types.h"

#define ATA_PRIMARY_BASE   0x1F0
#define ATA_SECONDARY_BASE 0x170
#define ATA_PRIMARY_CTRL   0x3F6
#define ATA_SECONDARY_CTRL 0x376

/* ATA registers */
#define ATA_REG_DATA       0x00
#define ATA_REG_ERROR      0x01
#define ATA_REG_FEATURES   0x01
#define ATA_REG_SECCOUNT   0x02
#define ATA_REG_LBA_LOW    0x03
#define ATA_REG_LBA_MID    0x04
#define ATA_REG_LBA_HIGH   0x05
#define ATA_REG_DRIVE      0x06
#define ATA_REG_STATUS     0x07
#define ATA_REG_COMMAND    0x07

/* ATA commands */
#define ATA_CMD_READ_SECTORS  0x20
#define ATA_CMD_WRITE_SECTORS 0x30
#define ATA_CMD_IDENTIFY      0xEC
#define ATA_CMD_CACHE_FLUSH   0xE7

/* ATA status bits */
#define ATA_STATUS_BSY  0x80
#define ATA_STATUS_DRDY 0x40
#define ATA_STATUS_DRQ  0x08
#define ATA_STATUS_ERR  0x01

typedef struct {
    u16 base;
    u16 ctrl;
    u8 drive;
    u8 present;
    u32 sectors;
    char model[41];
    char serial[21];
} ata_device_t;

static ata_device_t ata_devices[4];

static inline void outb(u16 port, u8 val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline u8 inb(u16 port) {
    u8 val;
    __asm__ volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static inline void outw(u16 port, u16 val) {
    __asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline u16 inw(u16 port) {
    u16 val;
    __asm__ volatile("inw %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static void ata_delay(u16 base) {
    /* 400ns delay by reading status 4 times */
    inb(base + ATA_REG_STATUS);
    inb(base + ATA_REG_STATUS);
    inb(base + ATA_REG_STATUS);
    inb(base + ATA_REG_STATUS);
}

static u8 ata_wait_ready(u16 base) {
    u32 timeout = 1000000;
    u8 status;
    
    do {
        status = inb(base + ATA_REG_STATUS);
        if (!(status & ATA_STATUS_BSY) && (status & ATA_STATUS_DRDY)) {
            return 1;
        }
        timeout--;
    } while (timeout > 0);
    
    return 0;
}

static u8 ata_wait_drq(u16 base) {
    u32 timeout = 1000000;
    u8 status;
    
    do {
        status = inb(base + ATA_REG_STATUS);
        if (status & ATA_STATUS_DRQ) {
            return 1;
        }
        if (status & ATA_STATUS_ERR) {
            return 0;
        }
        timeout--;
    } while (timeout > 0);
    
    return 0;
}

static void ata_string_swap(char* str, u32 len) {
    for (u32 i = 0; i < len; i += 2) {
        char tmp = str[i];
        str[i] = str[i + 1];
        str[i + 1] = tmp;
    }
}

static u8 ata_identify(ata_device_t* dev) {
    u16 base = dev->base;
    
    /* Select drive */
    outb(base + ATA_REG_DRIVE, 0xA0 | (dev->drive << 4));
    ata_delay(base);
    
    /* Send identify command */
    outb(base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    ata_delay(base);
    
    /* Check if drive exists */
    if (inb(base + ATA_REG_STATUS) == 0) {
        return 0;
    }
    
    /* Wait for response */
    if (!ata_wait_drq(base)) {
        return 0;
    }
    
    /* Read identify data */
    u16 identify[256];
    for (u16 i = 0; i < 256; i++) {
        identify[i] = inw(base + ATA_REG_DATA);
    }
    
    /* Extract information */
    dev->sectors = *(u32*)&identify[60];
    
    /* Model string */
    for (u8 i = 0; i < 40; i++) {
        dev->model[i] = ((char*)&identify[27])[i];
    }
    dev->model[40] = 0;
    ata_string_swap(dev->model, 40);
    
    /* Serial string */
    for (u8 i = 0; i < 20; i++) {
        dev->serial[i] = ((char*)&identify[10])[i];
    }
    dev->serial[20] = 0;
    ata_string_swap(dev->serial, 20);
    
    dev->present = 1;
    return 1;
}

void ata_init(void) {
    /* Initialize primary master */
    ata_devices[0].base = ATA_PRIMARY_BASE;
    ata_devices[0].ctrl = ATA_PRIMARY_CTRL;
    ata_devices[0].drive = 0;
    ata_devices[0].present = 0;
    
    /* Initialize primary slave */
    ata_devices[1].base = ATA_PRIMARY_BASE;
    ata_devices[1].ctrl = ATA_PRIMARY_CTRL;
    ata_devices[1].drive = 1;
    ata_devices[1].present = 0;
    
    /* Initialize secondary master */
    ata_devices[2].base = ATA_SECONDARY_BASE;
    ata_devices[2].ctrl = ATA_SECONDARY_CTRL;
    ata_devices[2].drive = 0;
    ata_devices[2].present = 0;
    
    /* Initialize secondary slave */
    ata_devices[3].base = ATA_SECONDARY_BASE;
    ata_devices[3].ctrl = ATA_SECONDARY_CTRL;
    ata_devices[3].drive = 1;
    ata_devices[3].present = 0;
    
    /* Identify all drives */
    for (u8 i = 0; i < 4; i++) {
        ata_identify(&ata_devices[i]);
    }
}

u8 ata_read_sectors(u8 drive, u32 lba, u8 count, void* buffer) {
    if (drive >= 4 || !ata_devices[drive].present) {
        return 0;
    }
    
    ata_device_t* dev = &ata_devices[drive];
    u16 base = dev->base;
    u16* buf = (u16*)buffer;
    
    /* Wait for drive ready */
    if (!ata_wait_ready(base)) {
        return 0;
    }
    
    /* Select drive and set LBA mode */
    outb(base + ATA_REG_DRIVE, 0xE0 | (dev->drive << 4) | ((lba >> 24) & 0x0F));
    
    /* Set parameters */
    outb(base + ATA_REG_SECCOUNT, count);
    outb(base + ATA_REG_LBA_LOW, lba & 0xFF);
    outb(base + ATA_REG_LBA_MID, (lba >> 8) & 0xFF);
    outb(base + ATA_REG_LBA_HIGH, (lba >> 16) & 0xFF);
    
    /* Send read command */
    outb(base + ATA_REG_COMMAND, ATA_CMD_READ_SECTORS);
    
    /* Read sectors */
    for (u8 sector = 0; sector < count; sector++) {
        if (!ata_wait_drq(base)) {
            return 0;
        }
        
        /* Read 256 words (512 bytes) */
        for (u16 i = 0; i < 256; i++) {
            buf[sector * 256 + i] = inw(base + ATA_REG_DATA);
        }
    }
    
    return 1;
}

u8 ata_write_sectors(u8 drive, u32 lba, u8 count, const void* buffer) {
    if (drive >= 4 || !ata_devices[drive].present) {
        return 0;
    }
    
    ata_device_t* dev = &ata_devices[drive];
    u16 base = dev->base;
    const u16* buf = (const u16*)buffer;
    
    /* Wait for drive ready */
    if (!ata_wait_ready(base)) {
        return 0;
    }
    
    /* Select drive and set LBA mode */
    outb(base + ATA_REG_DRIVE, 0xE0 | (dev->drive << 4) | ((lba >> 24) & 0x0F));
    
    /* Set parameters */
    outb(base + ATA_REG_SECCOUNT, count);
    outb(base + ATA_REG_LBA_LOW, lba & 0xFF);
    outb(base + ATA_REG_LBA_MID, (lba >> 8) & 0xFF);
    outb(base + ATA_REG_LBA_HIGH, (lba >> 16) & 0xFF);
    
    /* Send write command */
    outb(base + ATA_REG_COMMAND, ATA_CMD_WRITE_SECTORS);
    
    /* Write sectors */
    for (u8 sector = 0; sector < count; sector++) {
        if (!ata_wait_drq(base)) {
            return 0;
        }
        
        /* Write 256 words (512 bytes) */
        for (u16 i = 0; i < 256; i++) {
            outw(base + ATA_REG_DATA, buf[sector * 256 + i]);
        }
        
        /* Flush cache */
        outb(base + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
        ata_wait_ready(base);
    }
    
    return 1;
}

ata_device_t* ata_get_device(u8 drive) {
    if (drive < 4 && ata_devices[drive].present) {
        return &ata_devices[drive];
    }
    return 0;
}

u8 ata_get_device_count(void) {
    u8 count = 0;
    for (u8 i = 0; i < 4; i++) {
        if (ata_devices[i].present) {
            count++;
        }
    }
    return count;
}
