/*
 * floppy.c – x86 82077AA floppy disk controller
 */

#include "kernel/types.h"

/* Floppy controller registers */
#define FDC_STATUS_A    0x3F0  /* Status register A (PS/2 only) */
#define FDC_STATUS_B    0x3F1  /* Status register B (PS/2 only) */
#define FDC_DOR         0x3F2  /* Digital Output Register */
#define FDC_TDR         0x3F3  /* Tape Drive Register */
#define FDC_MSR         0x3F4  /* Main Status Register */
#define FDC_DSR         0x3F4  /* Data Rate Select Register (write) */
#define FDC_DATA        0x3F5  /* Data FIFO */
#define FDC_DIR         0x3F7  /* Digital Input Register (read) */
#define FDC_CCR         0x3F7  /* Configuration Control Register (write) */

/* DMA controller for floppy */
#define DMA_ADDR        0x04
#define DMA_COUNT       0x05
#define DMA_PAGE        0x81
#define DMA_MODE        0x0B
#define DMA_CLEAR       0x0C
#define DMA_MASK        0x0A

/* Main Status Register bits */
#define MSR_DRIVE0_BUSY 0x01
#define MSR_DRIVE1_BUSY 0x02
#define MSR_DRIVE2_BUSY 0x04
#define MSR_DRIVE3_BUSY 0x08
#define MSR_COMMAND_BSY 0x10
#define MSR_NON_DMA     0x20
#define MSR_DATA_INPUT  0x40
#define MSR_DATA_READY  0x80

/* Digital Output Register bits */
#define DOR_DRIVE_SEL   0x03  /* Drive select mask */
#define DOR_RESET       0x04  /* Controller reset (active low) */
#define DOR_IRQ_DMA     0x08  /* Enable IRQ and DMA */
#define DOR_MOTOR_A     0x10  /* Motor A enable */
#define DOR_MOTOR_B     0x20  /* Motor B enable */
#define DOR_MOTOR_C     0x40  /* Motor C enable */
#define DOR_MOTOR_D     0x80  /* Motor D enable */

/* Floppy commands */
#define CMD_READ_TRACK      0x02
#define CMD_SPECIFY         0x03
#define CMD_SENSE_STATUS    0x04
#define CMD_WRITE_SECTOR    0x05
#define CMD_READ_SECTOR     0x06
#define CMD_RECALIBRATE     0x07
#define CMD_SENSE_INTERRUPT 0x08
#define CMD_WRITE_DEL_SECT  0x09
#define CMD_READ_ID         0x0A
#define CMD_READ_DEL_SECT   0x0C
#define CMD_FORMAT_TRACK    0x0D
#define CMD_SEEK            0x0F
#define CMD_VERSION         0x10
#define CMD_SCAN_EQUAL      0x11
#define CMD_PERPENDICULAR   0x12
#define CMD_CONFIGURE       0x13
#define CMD_LOCK            0x14
#define CMD_VERIFY          0x16
#define CMD_SCAN_LOW_EQUAL  0x19
#define CMD_SCAN_HIGH_EQUAL 0x1D

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

static const floppy_geometry_t floppy_types[] = {
    {18, 2, 80, 0x1B, 0x00},  /* 1.44MB */
    {15, 2, 80, 0x1B, 0x00},  /* 1.2MB */
    {9,  2, 80, 0x1B, 0x02},  /* 720KB */
    {9,  2, 40, 0x1B, 0x02}   /* 360KB */
};

static volatile u8 floppy_irq_received = 0;
static u8 floppy_motor_ticks = 0;
static u8 current_drive = 0;
static u8 floppy_buffer[512] __attribute__((aligned(4)));

static inline void outb(u16 port, u8 val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline u8 inb(u16 port) {
    u8 val;
    __asm__ volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static void floppy_wait_irq(void) {
    while (!floppy_irq_received) {
        __asm__ volatile("hlt");
    }
    floppy_irq_received = 0;
}

static u8 floppy_read_data(void) {
    u32 timeout = 1000000;
    while (timeout-- && !(inb(FDC_MSR) & MSR_DATA_READY));
    return inb(FDC_DATA);
}

static void floppy_write_data(u8 data) {
    u32 timeout = 1000000;
    while (timeout-- && !(inb(FDC_MSR) & MSR_DATA_READY));
    outb(FDC_DATA, data);
}

static void floppy_send_command(u8 cmd) {
    u32 timeout = 1000000;
    while (timeout-- && !(inb(FDC_MSR) & MSR_DATA_READY));
    outb(FDC_DATA, cmd);
}

static void floppy_motor_on(u8 drive) {
    if (drive > 3) return;
    
    u8 motor_bit = DOR_MOTOR_A << drive;
    outb(FDC_DOR, DOR_RESET | DOR_IRQ_DMA | motor_bit | drive);
    
    /* Wait for motor to spin up */
    extern void timer_delay(u32 ms);
    timer_delay(500);
}

static void floppy_motor_off(u8 drive) {
    if (drive > 3) return;
    outb(FDC_DOR, DOR_RESET | DOR_IRQ_DMA | drive);
}

static void floppy_reset(void) {
    /* Reset controller */
    outb(FDC_DOR, 0);
    outb(FDC_DOR, DOR_RESET | DOR_IRQ_DMA);
    
    floppy_wait_irq();
    
    /* Sense interrupt status for all drives */
    for (u8 i = 0; i < 4; i++) {
        floppy_send_command(CMD_SENSE_INTERRUPT);
        floppy_read_data(); /* ST0 */
        floppy_read_data(); /* PCN */
    }
    
    /* Configure controller */
    floppy_send_command(CMD_CONFIGURE);
    floppy_write_data(0);    /* Required */
    floppy_write_data(0x58); /* Implied seek, FIFO on, threshold 8 */
    floppy_write_data(0);    /* Precompensation */
    
    /* Specify drive timings */
    floppy_send_command(CMD_SPECIFY);
    floppy_write_data(0xDF); /* SRT=3ms, HUT=240ms */
    floppy_write_data(0x02); /* HLT=16ms, ND=0 */
}

static u8 floppy_seek(u8 drive, u8 track) {
    if (drive > 3) return 0;
    
    floppy_motor_on(drive);
    
    for (u8 attempt = 0; attempt < 3; attempt++) {
        floppy_send_command(CMD_SEEK);
        floppy_write_data((drive & 3));
        floppy_write_data(track);
        
        floppy_wait_irq();
        
        /* Check if seek completed */
        floppy_send_command(CMD_SENSE_INTERRUPT);
        u8 st0 = floppy_read_data();
        u8 pcn = floppy_read_data();
        
        if (pcn == track && !(st0 & 0xC0)) {
            return 1; /* Success */
        }
    }
    
    return 0; /* Failed */
}

static u8 floppy_calibrate(u8 drive) {
    if (drive > 3) return 0;
    
    floppy_motor_on(drive);
    
    for (u8 attempt = 0; attempt < 3; attempt++) {
        floppy_send_command(CMD_RECALIBRATE);
        floppy_write_data(drive & 3);
        
        floppy_wait_irq();
        
        /* Check if calibration completed */
        floppy_send_command(CMD_SENSE_INTERRUPT);
        u8 st0 = floppy_read_data();
        u8 pcn = floppy_read_data();
        
        if (pcn == 0 && !(st0 & 0xC0)) {
            return 1; /* Success */
        }
    }
    
    return 0; /* Failed */
}

static void floppy_setup_dma(u8 read, u32 addr, u16 count) {
    /* Disable DMA channel 2 */
    outb(DMA_MASK, 0x06);
    
    /* Clear flip-flop */
    outb(DMA_CLEAR, 0);
    
    /* Set mode */
    if (read) {
        outb(DMA_MODE, 0x46); /* Single transfer, increment, read */
    } else {
        outb(DMA_MODE, 0x4A); /* Single transfer, increment, write */
    }
    
    /* Set address */
    outb(DMA_ADDR, addr & 0xFF);
    outb(DMA_ADDR, (addr >> 8) & 0xFF);
    outb(DMA_PAGE, (addr >> 16) & 0xFF);
    
    /* Set count */
    count--;
    outb(DMA_COUNT, count & 0xFF);
    outb(DMA_COUNT, (count >> 8) & 0xFF);
    
    /* Enable DMA channel 2 */
    outb(DMA_MASK, 0x02);
}

void floppy_init(void) {
    /* Reset controller */
    floppy_reset();
    
    /* Set data rate for 1.44MB */
    outb(FDC_CCR, 0x00);
    
    current_drive = 0;
}

u8 floppy_read_sector(u8 drive, u8 track, u8 head, u8 sector, void* buffer) {
    if (drive > 3) return 0;
    
    /* Setup DMA for read */
    floppy_setup_dma(1, (u32)floppy_buffer, 512);
    
    /* Seek to track */
    if (!floppy_seek(drive, track)) {
        return 0;
    }
    
    /* Read sector command */
    floppy_send_command(CMD_READ_SECTOR | 0x80 | 0x40); /* MT=1, MFM=1 */
    floppy_write_data((head << 2) | drive);
    floppy_write_data(track);
    floppy_write_data(head);
    floppy_write_data(sector);
    floppy_write_data(2);    /* 512 bytes per sector */
    floppy_write_data(18);   /* Sectors per track */
    floppy_write_data(0x1B); /* Gap 3 length */
    floppy_write_data(0xFF); /* Data length */
    
    floppy_wait_irq();
    
    /* Read result */
    u8 st0 = floppy_read_data();
    u8 st1 = floppy_read_data();
    u8 st2 = floppy_read_data();
    floppy_read_data(); /* C */
    floppy_read_data(); /* H */
    floppy_read_data(); /* R */
    floppy_read_data(); /* N */
    
    /* Check for errors */
    if (st0 & 0xC0 || st1 || st2) {
        return 0;
    }
    
    /* Copy data from DMA buffer */
    for (u16 i = 0; i < 512; i++) {
        ((u8*)buffer)[i] = floppy_buffer[i];
    }
    
    return 1;
}

u8 floppy_write_sector(u8 drive, u8 track, u8 head, u8 sector, const void* buffer) {
    if (drive > 3) return 0;
    
    /* Copy data to DMA buffer */
    for (u16 i = 0; i < 512; i++) {
        floppy_buffer[i] = ((const u8*)buffer)[i];
    }
    
    /* Setup DMA for write */
    floppy_setup_dma(0, (u32)floppy_buffer, 512);
    
    /* Seek to track */
    if (!floppy_seek(drive, track)) {
        return 0;
    }
    
    /* Write sector command */
    floppy_send_command(CMD_WRITE_SECTOR | 0x80 | 0x40); /* MT=1, MFM=1 */
    floppy_write_data((head << 2) | drive);
    floppy_write_data(track);
    floppy_write_data(head);
    floppy_write_data(sector);
    floppy_write_data(2);    /* 512 bytes per sector */
    floppy_write_data(18);   /* Sectors per track */
    floppy_write_data(0x1B); /* Gap 3 length */
    floppy_write_data(0xFF); /* Data length */
    
    floppy_wait_irq();
    
    /* Read result */
    u8 st0 = floppy_read_data();
    u8 st1 = floppy_read_data();
    u8 st2 = floppy_read_data();
    floppy_read_data(); /* C */
    floppy_read_data(); /* H */
    floppy_read_data(); /* R */
    floppy_read_data(); /* N */
    
    /* Check for errors */
    if (st0 & 0xC0 || st1 || st2) {
        return 0;
    }
    
    return 1;
}

u8 floppy_detect_type(u8 drive) {
    extern u8 cmos_get_floppy_type(void);
    
    u8 cmos_type = cmos_get_floppy_type();
    
    if (drive == 0) {
        return (cmos_type >> 4) & 0x0F;
    } else if (drive == 1) {
        return cmos_type & 0x0F;
    }
    
    return 0; /* No floppy */
}

void floppy_irq_handler(void) {
    floppy_irq_received = 1;
}

const floppy_geometry_t* floppy_get_geometry(u8 type) {
    if (type < 4) {
        return &floppy_types[type];
    }
    return 0;
}
