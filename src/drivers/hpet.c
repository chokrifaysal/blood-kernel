/*
 * hpet.c – x86 High Precision Event Timer
 */

#include "kernel/types.h"

#define HPET_GENERAL_CAPS_ID    0x000
#define HPET_GENERAL_CONFIG     0x010
#define HPET_GENERAL_INT_STATUS 0x020
#define HPET_MAIN_COUNTER       0x0F0

#define HPET_TIMER0_CONFIG      0x100
#define HPET_TIMER0_COMPARATOR  0x108
#define HPET_TIMER0_FSB_ROUTE   0x110

#define HPET_TIMER1_CONFIG      0x120
#define HPET_TIMER1_COMPARATOR  0x128
#define HPET_TIMER1_FSB_ROUTE   0x130

#define HPET_TIMER2_CONFIG      0x140
#define HPET_TIMER2_COMPARATOR  0x148
#define HPET_TIMER2_FSB_ROUTE   0x150

/* Configuration register bits */
#define HPET_CFG_ENABLE         0x001
#define HPET_CFG_LEGACY         0x002

/* Timer configuration bits */
#define HPET_TN_INT_TYPE        0x002  /* Level triggered */
#define HPET_TN_INT_ENB         0x004  /* Interrupt enable */
#define HPET_TN_TYPE            0x008  /* Periodic */
#define HPET_TN_PER_INT_CAP     0x010  /* Periodic interrupt capable */
#define HPET_TN_SIZE_CAP        0x020  /* 64-bit capable */
#define HPET_TN_SETVAL          0x040  /* Set accumulator */
#define HPET_TN_32BIT           0x100  /* 32-bit mode */
#define HPET_TN_ROUTE           0x3E00 /* Interrupt route */
#define HPET_TN_FSB             0x4000 /* FSB interrupt delivery */
#define HPET_TN_FSB_CAP         0x8000 /* FSB delivery capable */

typedef struct {
    u64 base_address;
    u32 frequency;
    u8 num_timers;
    u8 counter_size;
    u8 legacy_capable;
    u8 initialized;
} hpet_info_t;

static hpet_info_t hpet_info;

static inline u64 hpet_read64(u32 offset) {
    volatile u64* addr = (volatile u64*)(hpet_info.base_address + offset);
    return *addr;
}

static inline void hpet_write64(u32 offset, u64 value) {
    volatile u64* addr = (volatile u64*)(hpet_info.base_address + offset);
    *addr = value;
}

static inline u32 hpet_read32(u32 offset) {
    volatile u32* addr = (volatile u32*)(hpet_info.base_address + offset);
    return *addr;
}

static inline void hpet_write32(u32 offset, u32 value) {
    volatile u32* addr = (volatile u32*)(hpet_info.base_address + offset);
    *addr = value;
}

u8 hpet_init(u64 base_address) {
    hpet_info.base_address = base_address;
    
    /* Read capabilities */
    u64 caps = hpet_read64(HPET_GENERAL_CAPS_ID);
    
    /* Extract information */
    hpet_info.frequency = 1000000000000000ULL / (caps >> 32); /* femtoseconds to Hz */
    hpet_info.num_timers = ((caps >> 8) & 0x1F) + 1;
    hpet_info.counter_size = (caps & (1 << 13)) ? 64 : 32;
    hpet_info.legacy_capable = (caps & (1 << 15)) != 0;
    
    /* Disable HPET */
    hpet_write64(HPET_GENERAL_CONFIG, 0);
    
    /* Reset main counter */
    hpet_write64(HPET_MAIN_COUNTER, 0);
    
    /* Configure timers */
    for (u8 i = 0; i < hpet_info.num_timers && i < 3; i++) {
        u32 timer_offset = HPET_TIMER0_CONFIG + (i * 0x20);
        
        /* Read timer capabilities */
        u64 timer_config = hpet_read64(timer_offset);
        
        /* Disable timer */
        timer_config &= ~HPET_TN_INT_ENB;
        hpet_write64(timer_offset, timer_config);
        
        /* Clear comparator */
        hpet_write64(timer_offset + 8, 0);
    }
    
    /* Enable HPET */
    u64 config = HPET_CFG_ENABLE;
    if (hpet_info.legacy_capable) {
        config |= HPET_CFG_LEGACY;
    }
    hpet_write64(HPET_GENERAL_CONFIG, config);
    
    hpet_info.initialized = 1;
    return 1;
}

u64 hpet_get_counter(void) {
    if (!hpet_info.initialized) return 0;
    
    if (hpet_info.counter_size == 64) {
        return hpet_read64(HPET_MAIN_COUNTER);
    } else {
        return hpet_read32(HPET_MAIN_COUNTER);
    }
}

u64 hpet_get_frequency(void) {
    return hpet_info.frequency;
}

void hpet_udelay(u32 microseconds) {
    if (!hpet_info.initialized) return;
    
    u64 ticks_per_us = hpet_info.frequency / 1000000;
    u64 target_ticks = ticks_per_us * microseconds;
    u64 start = hpet_get_counter();
    
    while ((hpet_get_counter() - start) < target_ticks) {
        __asm__ volatile("pause");
    }
}

void hpet_ndelay(u32 nanoseconds) {
    if (!hpet_info.initialized) return;
    
    u64 ticks_per_ns = hpet_info.frequency / 1000000000;
    if (ticks_per_ns == 0) ticks_per_ns = 1;
    
    u64 target_ticks = ticks_per_ns * nanoseconds;
    u64 start = hpet_get_counter();
    
    while ((hpet_get_counter() - start) < target_ticks) {
        __asm__ volatile("pause");
    }
}

u8 hpet_setup_timer(u8 timer_num, u32 period_us, u8 periodic, u8 irq) {
    if (!hpet_info.initialized || timer_num >= hpet_info.num_timers || timer_num >= 3) {
        return 0;
    }
    
    u32 timer_offset = HPET_TIMER0_CONFIG + (timer_num * 0x20);
    
    /* Read timer capabilities */
    u64 timer_config = hpet_read64(timer_offset);
    
    /* Check if periodic mode is supported */
    if (periodic && !(timer_config & HPET_TN_PER_INT_CAP)) {
        return 0;
    }
    
    /* Disable timer */
    timer_config &= ~HPET_TN_INT_ENB;
    hpet_write64(timer_offset, timer_config);
    
    /* Configure timer */
    timer_config = 0;
    
    if (periodic) {
        timer_config |= HPET_TN_TYPE | HPET_TN_SETVAL;
    }
    
    /* Set interrupt routing */
    timer_config |= (irq << 9) & HPET_TN_ROUTE;
    
    /* Enable level-triggered interrupts */
    timer_config |= HPET_TN_INT_TYPE;
    
    /* Set 32-bit mode if counter is 32-bit */
    if (hpet_info.counter_size == 32) {
        timer_config |= HPET_TN_32BIT;
    }
    
    hpet_write64(timer_offset, timer_config);
    
    /* Calculate comparator value */
    u64 ticks_per_us = hpet_info.frequency / 1000000;
    u64 comparator_value = ticks_per_us * period_us;
    
    if (periodic) {
        /* For periodic timers, set the period */
        hpet_write64(timer_offset + 8, comparator_value);
    } else {
        /* For one-shot timers, set absolute time */
        u64 current = hpet_get_counter();
        hpet_write64(timer_offset + 8, current + comparator_value);
    }
    
    /* Enable timer interrupt */
    timer_config |= HPET_TN_INT_ENB;
    hpet_write64(timer_offset, timer_config);
    
    return 1;
}

void hpet_disable_timer(u8 timer_num) {
    if (!hpet_info.initialized || timer_num >= hpet_info.num_timers || timer_num >= 3) {
        return;
    }
    
    u32 timer_offset = HPET_TIMER0_CONFIG + (timer_num * 0x20);
    u64 timer_config = hpet_read64(timer_offset);
    
    timer_config &= ~HPET_TN_INT_ENB;
    hpet_write64(timer_offset, timer_config);
}

u8 hpet_is_initialized(void) {
    return hpet_info.initialized;
}

u8 hpet_get_num_timers(void) {
    return hpet_info.num_timers;
}

u8 hpet_is_legacy_capable(void) {
    return hpet_info.legacy_capable;
}

u64 hpet_get_timestamp_ns(void) {
    if (!hpet_info.initialized) return 0;
    
    u64 counter = hpet_get_counter();
    return (counter * 1000000000ULL) / hpet_info.frequency;
}

u64 hpet_get_timestamp_us(void) {
    if (!hpet_info.initialized) return 0;
    
    u64 counter = hpet_get_counter();
    return (counter * 1000000ULL) / hpet_info.frequency;
}

void hpet_calibrate_tsc(u64* tsc_frequency) {
    if (!hpet_info.initialized || !tsc_frequency) return;
    
    /* Measure TSC frequency using HPET as reference */
    u64 hpet_start = hpet_get_counter();
    u64 tsc_start, tsc_end;
    
    __asm__ volatile("rdtsc" : "=A"(tsc_start));
    
    /* Wait for 10ms */
    hpet_udelay(10000);
    
    __asm__ volatile("rdtsc" : "=A"(tsc_end));
    u64 hpet_end = hpet_get_counter();
    
    u64 hpet_elapsed = hpet_end - hpet_start;
    u64 tsc_elapsed = tsc_end - tsc_start;
    
    /* Calculate TSC frequency */
    *tsc_frequency = (tsc_elapsed * hpet_info.frequency) / hpet_elapsed;
}

void hpet_irq_handler(u8 timer_num) {
    if (!hpet_info.initialized) return;
    
    /* Clear interrupt status */
    u32 status = hpet_read32(HPET_GENERAL_INT_STATUS);
    if (status & (1 << timer_num)) {
        hpet_write32(HPET_GENERAL_INT_STATUS, 1 << timer_num);
    }
}
