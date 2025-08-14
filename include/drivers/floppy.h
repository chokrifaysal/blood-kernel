/*
 * floppy.h – x86 floppy disk controller
 */

#ifndef FLOPPY_H
#define FLOPPY_H

#include "kernel/types.h"

/* Floppy disk types */
#define FLOPPY_144M     0  /* 1.44MB 3.5" */
#define FLOPPY_12M      1  /* 1.2MB 5.25" */
#define FLOPPY_720K     2  /* 720KB 3.5" */
#define FLOPPY_360K     3  /* 360KB 5.25" */

typedef struct {
    u16 sectors_per_track;
    u16 heads;
    u16 tracks;
    u8 gap3_length;
    u8 data_rate;
} floppy_geometry_t;

/* Core functions */
void floppy_init(void);

/* I/O functions */
u8 floppy_read_sector(u8 drive, u8 track, u8 head, u8 sector, void* buffer);
u8 floppy_write_sector(u8 drive, u8 track, u8 head, u8 sector, const void* buffer);

/* Detection functions */
u8 floppy_detect_type(u8 drive);
const floppy_geometry_t* floppy_get_geometry(u8 type);

/* Interrupt handler */
void floppy_irq_handler(void);

#endif
