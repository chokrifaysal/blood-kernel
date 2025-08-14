/*
 * pic.c – x86 8259A Programmable Interrupt Controller
 */

#include "kernel/types.h"

#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

#define PIC_EOI      0x20

/* ICW1 bits */
#define ICW1_ICW4    0x01  /* ICW4 needed */
#define ICW1_SINGLE  0x02  /* Single (cascade) mode */
#define ICW1_INTERVAL4 0x04  /* Call address interval 4 (8) */
#define ICW1_LEVEL   0x08  /* Level triggered (edge) mode */
#define ICW1_INIT    0x10  /* Initialization */

/* ICW4 bits */
#define ICW4_8086    0x01  /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO    0x02  /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE 0x08  /* Buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C /* Buffered mode/master */
#define ICW4_SFNM    0x10  /* Special fully nested (not) */

static u16 pic_irq_mask = 0xFFFF; /* All IRQs masked initially */

static inline void outb(u16 port, u8 val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline u8 inb(u16 port) {
    u8 val;
    __asm__ volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static void io_wait(void) {
    /* Wait for I/O operation to complete */
    outb(0x80, 0);
}

void pic_init(void) {
    u8 mask1, mask2;
    
    /* Save current masks */
    mask1 = inb(PIC1_DATA);
    mask2 = inb(PIC2_DATA);
    
    /* Start initialization sequence */
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    
    /* Set vector offsets */
    outb(PIC1_DATA, 0x20);  /* Master PIC vector offset: 32 */
    io_wait();
    outb(PIC2_DATA, 0x28);  /* Slave PIC vector offset: 40 */
    io_wait();
    
    /* Configure cascade */
    outb(PIC1_DATA, 4);     /* Tell master PIC that slave is at IRQ2 */
    io_wait();
    outb(PIC2_DATA, 2);     /* Tell slave PIC its cascade identity */
    io_wait();
    
    /* Set mode */
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();
    
    /* Restore masks */
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

void pic_enable_irq(u8 irq) {
    if (irq >= 16) return;
    
    pic_irq_mask &= ~(1 << irq);
    
    if (irq < 8) {
        outb(PIC1_DATA, pic_irq_mask & 0xFF);
    } else {
        outb(PIC2_DATA, (pic_irq_mask >> 8) & 0xFF);
        /* Enable cascade IRQ2 on master */
        pic_irq_mask &= ~(1 << 2);
        outb(PIC1_DATA, pic_irq_mask & 0xFF);
    }
}

void pic_disable_irq(u8 irq) {
    if (irq >= 16) return;
    
    pic_irq_mask |= (1 << irq);
    
    if (irq < 8) {
        outb(PIC1_DATA, pic_irq_mask & 0xFF);
    } else {
        outb(PIC2_DATA, (pic_irq_mask >> 8) & 0xFF);
    }
}

void pic_send_eoi(u8 irq) {
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}

u16 pic_get_irr(void) {
    /* Get Interrupt Request Register */
    outb(PIC1_COMMAND, 0x0A);
    outb(PIC2_COMMAND, 0x0A);
    return (inb(PIC2_COMMAND) << 8) | inb(PIC1_COMMAND);
}

u16 pic_get_isr(void) {
    /* Get In-Service Register */
    outb(PIC1_COMMAND, 0x0B);
    outb(PIC2_COMMAND, 0x0B);
    return (inb(PIC2_COMMAND) << 8) | inb(PIC1_COMMAND);
}

void pic_mask_all(void) {
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
    pic_irq_mask = 0xFFFF;
}

void pic_unmask_all(void) {
    outb(PIC1_DATA, 0x00);
    outb(PIC2_DATA, 0x00);
    pic_irq_mask = 0x0000;
}

u8 pic_is_spurious_irq(u8 irq) {
    if (irq == 7) {
        /* Check if IRQ7 is spurious */
        outb(PIC1_COMMAND, 0x0B);
        return !(inb(PIC1_COMMAND) & 0x80);
    } else if (irq == 15) {
        /* Check if IRQ15 is spurious */
        outb(PIC2_COMMAND, 0x0B);
        if (!(inb(PIC2_COMMAND) & 0x80)) {
            /* Send EOI to master only */
            outb(PIC1_COMMAND, PIC_EOI);
            return 1;
        }
    }
    return 0;
}

void pic_set_priority(u8 irq) {
    if (irq >= 16) return;
    
    if (irq < 8) {
        outb(PIC1_COMMAND, 0xC0 | (irq & 7));
    } else {
        outb(PIC2_COMMAND, 0xC0 | ((irq - 8) & 7));
    }
}

u16 pic_get_mask(void) {
    return pic_irq_mask;
}

void pic_set_mask(u16 mask) {
    pic_irq_mask = mask;
    outb(PIC1_DATA, mask & 0xFF);
    outb(PIC2_DATA, (mask >> 8) & 0xFF);
}

void pic_disable(void) {
    /* Disable both PICs by masking all interrupts */
    pic_mask_all();
    
    /* Remap to unused vectors to avoid conflicts */
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    
    outb(PIC1_DATA, 0xF0);  /* Remap to vectors 0xF0-0xF7 */
    io_wait();
    outb(PIC2_DATA, 0xF8);  /* Remap to vectors 0xF8-0xFF */
    io_wait();
    
    outb(PIC1_DATA, 4);
    io_wait();
    outb(PIC2_DATA, 2);
    io_wait();
    
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();
    
    /* Mask all interrupts */
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

/* IRQ names for debugging */
static const char* irq_names[] = {
    "Timer",
    "Keyboard",
    "Cascade",
    "COM2",
    "COM1",
    "LPT2",
    "Floppy",
    "LPT1",
    "RTC",
    "Free",
    "Free",
    "Free",
    "PS/2 Mouse",
    "FPU",
    "Primary ATA",
    "Secondary ATA"
};

const char* pic_get_irq_name(u8 irq) {
    if (irq < 16) {
        return irq_names[irq];
    }
    return "Unknown";
}
