/*
 * pit.c – x86 8253/8254 Programmable Interval Timer
 */

#include "kernel/types.h"

#define PIT_CHANNEL0 0x40
#define PIT_CHANNEL1 0x41
#define PIT_CHANNEL2 0x42
#define PIT_COMMAND  0x43

#define PIT_FREQUENCY 1193182

/* Command register bits */
#define PIT_SELECT_CHANNEL0 (0 << 6)
#define PIT_SELECT_CHANNEL1 (1 << 6)
#define PIT_SELECT_CHANNEL2 (2 << 6)
#define PIT_ACCESS_LATCH    (0 << 4)
#define PIT_ACCESS_LOW      (1 << 4)
#define PIT_ACCESS_HIGH     (2 << 4)
#define PIT_ACCESS_BOTH     (3 << 4)
#define PIT_MODE_TERMINAL   (0 << 1)
#define PIT_MODE_ONESHOT    (1 << 1)
#define PIT_MODE_RATE       (2 << 1)
#define PIT_MODE_SQUARE     (3 << 1)
#define PIT_MODE_SOFT_STROBE (4 << 1)
#define PIT_MODE_HARD_STROBE (5 << 1)
#define PIT_BINARY          (0 << 0)
#define PIT_BCD             (1 << 0)

static volatile u32 pit_ticks = 0;
static u32 pit_frequency = 1000; /* Default 1 kHz */

static inline void outb(u16 port, u8 val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline u8 inb(u16 port) {
    u8 val;
    __asm__ volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

void pit_init(u32 frequency) {
    pit_frequency = frequency;
    
    /* Calculate divisor */
    u16 divisor = PIT_FREQUENCY / frequency;
    
    /* Send command byte */
    outb(PIT_COMMAND, PIT_SELECT_CHANNEL0 | PIT_ACCESS_BOTH | 
         PIT_MODE_RATE | PIT_BINARY);
    
    /* Send divisor */
    outb(PIT_CHANNEL0, divisor & 0xFF);
    outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);
}

void pit_irq_handler(void) {
    pit_ticks++;
}

u32 pit_get_ticks(void) {
    return pit_ticks;
}

u32 pit_get_frequency(void) {
    return pit_frequency;
}

void pit_delay(u32 ms) {
    u32 target = pit_ticks + (ms * pit_frequency / 1000);
    while (pit_ticks < target) {
        __asm__ volatile("hlt");
    }
}

void pit_delay_us(u32 us) {
    /* For microsecond delays, use busy wait with PIT readback */
    u32 target_us = us;
    
    while (target_us > 0) {
        /* Read current counter value */
        outb(PIT_COMMAND, PIT_SELECT_CHANNEL0 | PIT_ACCESS_LATCH);
        u8 low = inb(PIT_CHANNEL0);
        u8 high = inb(PIT_CHANNEL0);
        u16 current = (high << 8) | low;
        
        /* Wait for counter to change */
        u16 last = current;
        do {
            outb(PIT_COMMAND, PIT_SELECT_CHANNEL0 | PIT_ACCESS_LATCH);
            low = inb(PIT_CHANNEL0);
            high = inb(PIT_CHANNEL0);
            current = (high << 8) | low;
        } while (current == last && target_us > 0);
        
        /* Calculate elapsed time */
        u16 elapsed_ticks = (last > current) ? (last - current) : 
                           (65536 - current + last);
        u32 elapsed_us = (elapsed_ticks * 1000000UL) / PIT_FREQUENCY;
        
        if (elapsed_us >= target_us) {
            target_us = 0;
        } else {
            target_us -= elapsed_us;
        }
    }
}

void pit_beep(u32 frequency, u32 duration_ms) {
    if (frequency == 0) {
        /* Turn off speaker */
        u8 tmp = inb(0x61) & 0xFC;
        outb(0x61, tmp);
        return;
    }
    
    /* Calculate divisor for channel 2 */
    u16 divisor = PIT_FREQUENCY / frequency;
    
    /* Configure channel 2 for square wave */
    outb(PIT_COMMAND, PIT_SELECT_CHANNEL2 | PIT_ACCESS_BOTH | 
         PIT_MODE_SQUARE | PIT_BINARY);
    
    /* Send divisor */
    outb(PIT_CHANNEL2, divisor & 0xFF);
    outb(PIT_CHANNEL2, (divisor >> 8) & 0xFF);
    
    /* Enable speaker */
    u8 tmp = inb(0x61);
    if (tmp != (tmp | 3)) {
        outb(0x61, tmp | 3);
    }
    
    /* Wait for duration */
    if (duration_ms > 0) {
        pit_delay(duration_ms);
        
        /* Turn off speaker */
        tmp = inb(0x61) & 0xFC;
        outb(0x61, tmp);
    }
}

u64 pit_get_uptime_ms(void) {
    return (u64)pit_ticks * 1000 / pit_frequency;
}

u64 pit_get_uptime_us(void) {
    u64 ms = pit_get_uptime_ms();
    
    /* Get sub-millisecond precision */
    outb(PIT_COMMAND, PIT_SELECT_CHANNEL0 | PIT_ACCESS_LATCH);
    u8 low = inb(PIT_CHANNEL0);
    u8 high = inb(PIT_CHANNEL0);
    u16 current = (high << 8) | low;
    
    u16 divisor = PIT_FREQUENCY / pit_frequency;
    u16 elapsed_ticks = divisor - current;
    u32 sub_ms_us = (elapsed_ticks * 1000UL) / (PIT_FREQUENCY / 1000);
    
    return ms * 1000 + sub_ms_us;
}

void pit_calibrate_delay_loop(void) {
    /* Calibrate a delay loop for very short delays */
    u32 start_ticks = pit_ticks;
    volatile u32 loops = 0;
    
    /* Count loops for 10ms */
    while (pit_ticks < start_ticks + (pit_frequency / 100)) {
        loops++;
    }
    
    /* Store calibration result */
    static u32 loops_per_10ms = 0;
    loops_per_10ms = loops;
    (void)loops_per_10ms; /* Suppress warning */
}

void pit_reset(void) {
    pit_ticks = 0;
}

void pit_set_channel1(u16 divisor) {
    /* Channel 1 is typically used for DRAM refresh */
    outb(PIT_COMMAND, PIT_SELECT_CHANNEL1 | PIT_ACCESS_BOTH | 
         PIT_MODE_RATE | PIT_BINARY);
    outb(PIT_CHANNEL1, divisor & 0xFF);
    outb(PIT_CHANNEL1, (divisor >> 8) & 0xFF);
}

u16 pit_read_counter(u8 channel) {
    u8 command = (channel << 6) | PIT_ACCESS_LATCH;
    outb(PIT_COMMAND, command);
    
    u8 low = inb(PIT_CHANNEL0 + channel);
    u8 high = inb(PIT_CHANNEL0 + channel);
    
    return (high << 8) | low;
}

void pit_oneshot(u8 channel, u16 count) {
    u8 command = (channel << 6) | PIT_ACCESS_BOTH | PIT_MODE_ONESHOT | PIT_BINARY;
    outb(PIT_COMMAND, command);
    
    outb(PIT_CHANNEL0 + channel, count & 0xFF);
    outb(PIT_CHANNEL0 + channel, (count >> 8) & 0xFF);
}
