/*
 * timing_sync.c – x86 CPU timing/synchronization (TSC/HPET extensions)
 */

#include "kernel/types.h"

/* TSC MSRs */
#define MSR_TSC                         0x10
#define MSR_TSC_AUX                     0xC0000103
#define MSR_TSC_DEADLINE                0x6E0

/* HPET registers */
#define HPET_GENERAL_CAPS               0x00
#define HPET_GENERAL_CONFIG             0x10
#define HPET_GENERAL_INT_STATUS         0x20
#define HPET_MAIN_COUNTER               0xF0
#define HPET_TIMER0_CONFIG              0x100
#define HPET_TIMER0_COMPARATOR          0x108
#define HPET_TIMER0_FSB_ROUTE           0x110

/* HPET configuration bits */
#define HPET_CFG_ENABLE                 (1 << 0)
#define HPET_CFG_LEGACY_REPLACEMENT     (1 << 1)

/* Timer configuration bits */
#define HPET_TIMER_INT_TYPE_LEVEL       (1 << 1)
#define HPET_TIMER_INT_ENABLE           (1 << 2)
#define HPET_TIMER_TYPE_PERIODIC        (1 << 3)
#define HPET_TIMER_CAP_PERIODIC         (1 << 4)
#define HPET_TIMER_CAP_64BIT            (1 << 5)
#define HPET_TIMER_VAL_SET              (1 << 6)
#define HPET_TIMER_32BIT_MODE           (1 << 8)
#define HPET_TIMER_FSB_ENABLE           (1 << 14)

/* Synchronization primitives */
#define SYNC_BARRIER_MAGIC              0xDEADBEEF

typedef struct {
    volatile u32 count;
    volatile u32 sense;
    u32 total_cpus;
} barrier_t;

typedef struct {
    u64 frequency;
    u64 period_fs;
    u8 invariant;
    u8 reliable;
    u64 last_value;
    u64 drift_correction;
} tsc_info_t;

typedef struct {
    u64 base_address;
    u64 frequency;
    u64 period_fs;
    u8 num_timers;
    u8 counter_size;
    u8 legacy_replacement;
    u64 main_counter;
    u32 timer_interrupts[8];
} hpet_info_t;

typedef struct {
    u8 timing_sync_supported;
    u8 tsc_supported;
    u8 tsc_deadline_supported;
    u8 hpet_supported;
    u8 rdtscp_supported;
    tsc_info_t tsc;
    hpet_info_t hpet;
    barrier_t sync_barrier;
    u32 sync_operations;
    u32 timing_measurements;
    u64 last_sync_time;
    u64 max_sync_latency;
    u64 min_sync_latency;
    u32 cpu_id;
} timing_sync_info_t;

static timing_sync_info_t timing_sync_info;

extern u64 msr_read(u32 msr);
extern void msr_write(u32 msr, u64 value);
extern u8 msr_is_supported(void);
extern u64 inq(u16 port);
extern void outq(u16 port, u64 value);
extern u32 inl(u16 port);
extern void outl(u16 port, u32 value);

static u64 timing_sync_rdtsc(void) {
    u32 low, high;
    __asm__ volatile("rdtsc" : "=a"(low), "=d"(high));
    return ((u64)high << 32) | low;
}

static u64 timing_sync_rdtscp(u32* aux) {
    u32 low, high, cpu_id;
    __asm__ volatile("rdtscp" : "=a"(low), "=d"(high), "=c"(cpu_id));
    if (aux) *aux = cpu_id;
    return ((u64)high << 32) | low;
}

static void timing_sync_detect_tsc(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check for TSC support */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    timing_sync_info.tsc_supported = (edx & (1 << 4)) != 0;
    
    /* Check for invariant TSC */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x80000007));
    timing_sync_info.tsc.invariant = (edx & (1 << 8)) != 0;
    
    /* Check for TSC deadline timer */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    timing_sync_info.tsc_deadline_supported = (ecx & (1 << 24)) != 0;
    
    /* Check for RDTSCP */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x80000001));
    timing_sync_info.rdtscp_supported = (edx & (1 << 27)) != 0;
    
    if (timing_sync_info.tsc_supported) {
        /* Estimate TSC frequency */
        u64 start_tsc = timing_sync_rdtsc();
        
        /* Busy wait for a short period */
        for (volatile u32 i = 0; i < 1000000; i++);
        
        u64 end_tsc = timing_sync_rdtsc();
        timing_sync_info.tsc.frequency = (end_tsc - start_tsc) * 1000; /* Rough estimate */
        timing_sync_info.tsc.reliable = timing_sync_info.tsc.invariant;
    }
}

static void timing_sync_detect_hpet(void) {
    /* HPET detection would normally parse ACPI tables */
    /* For this implementation, assume standard HPET base address */
    timing_sync_info.hpet.base_address = 0xFED00000;
    
    /* Try to read HPET capabilities */
    volatile u64* hpet_base = (volatile u64*)timing_sync_info.hpet.base_address;
    u64 caps = hpet_base[HPET_GENERAL_CAPS >> 3];
    
    if (caps != 0xFFFFFFFFFFFFFFFF) {
        timing_sync_info.hpet_supported = 1;
        
        /* Extract HPET information */
        timing_sync_info.hpet.period_fs = caps >> 32;
        timing_sync_info.hpet.num_timers = ((caps >> 8) & 0x1F) + 1;
        timing_sync_info.hpet.counter_size = (caps & (1 << 13)) ? 64 : 32;
        
        /* Calculate frequency from period */
        if (timing_sync_info.hpet.period_fs > 0) {
            timing_sync_info.hpet.frequency = 1000000000000000ULL / timing_sync_info.hpet.period_fs;
        }
    }
}

void timing_sync_init(void) {
    timing_sync_info.timing_sync_supported = 0;
    timing_sync_info.tsc_supported = 0;
    timing_sync_info.tsc_deadline_supported = 0;
    timing_sync_info.hpet_supported = 0;
    timing_sync_info.rdtscp_supported = 0;
    timing_sync_info.sync_operations = 0;
    timing_sync_info.timing_measurements = 0;
    timing_sync_info.last_sync_time = 0;
    timing_sync_info.max_sync_latency = 0;
    timing_sync_info.min_sync_latency = 0xFFFFFFFFFFFFFFFF;
    timing_sync_info.cpu_id = 0;
    
    /* Initialize TSC info */
    timing_sync_info.tsc.frequency = 0;
    timing_sync_info.tsc.period_fs = 0;
    timing_sync_info.tsc.invariant = 0;
    timing_sync_info.tsc.reliable = 0;
    timing_sync_info.tsc.last_value = 0;
    timing_sync_info.tsc.drift_correction = 0;
    
    /* Initialize HPET info */
    timing_sync_info.hpet.base_address = 0;
    timing_sync_info.hpet.frequency = 0;
    timing_sync_info.hpet.period_fs = 0;
    timing_sync_info.hpet.num_timers = 0;
    timing_sync_info.hpet.counter_size = 0;
    timing_sync_info.hpet.legacy_replacement = 0;
    timing_sync_info.hpet.main_counter = 0;
    
    for (u8 i = 0; i < 8; i++) {
        timing_sync_info.hpet.timer_interrupts[i] = 0;
    }
    
    /* Initialize barrier */
    timing_sync_info.sync_barrier.count = 0;
    timing_sync_info.sync_barrier.sense = 0;
    timing_sync_info.sync_barrier.total_cpus = 1;
    
    timing_sync_detect_tsc();
    timing_sync_detect_hpet();
    
    if (timing_sync_info.tsc_supported || timing_sync_info.hpet_supported) {
        timing_sync_info.timing_sync_supported = 1;
    }
}

u8 timing_sync_is_supported(void) {
    return timing_sync_info.timing_sync_supported;
}

u8 timing_sync_is_tsc_supported(void) {
    return timing_sync_info.tsc_supported;
}

u8 timing_sync_is_hpet_supported(void) {
    return timing_sync_info.hpet_supported;
}

u8 timing_sync_is_tsc_invariant(void) {
    return timing_sync_info.tsc.invariant;
}

u8 timing_sync_is_rdtscp_supported(void) {
    return timing_sync_info.rdtscp_supported;
}

u64 timing_sync_get_tsc(void) {
    if (!timing_sync_info.tsc_supported) return 0;
    
    u64 tsc = timing_sync_rdtsc();
    timing_sync_info.tsc.last_value = tsc;
    return tsc;
}

u64 timing_sync_get_tsc_with_cpu_id(u32* cpu_id) {
    if (!timing_sync_info.rdtscp_supported) {
        if (cpu_id) *cpu_id = timing_sync_info.cpu_id;
        return timing_sync_get_tsc();
    }
    
    u64 tsc = timing_sync_rdtscp(cpu_id);
    timing_sync_info.tsc.last_value = tsc;
    return tsc;
}

u64 timing_sync_get_tsc_frequency(void) {
    return timing_sync_info.tsc.frequency;
}

void timing_sync_set_tsc_deadline(u64 deadline) {
    if (!timing_sync_info.tsc_deadline_supported || !msr_is_supported()) return;
    
    msr_write(MSR_TSC_DEADLINE, deadline);
}

u64 timing_sync_get_hpet_counter(void) {
    if (!timing_sync_info.hpet_supported) return 0;
    
    volatile u64* hpet_base = (volatile u64*)timing_sync_info.hpet.base_address;
    u64 counter = hpet_base[HPET_MAIN_COUNTER >> 3];
    timing_sync_info.hpet.main_counter = counter;
    return counter;
}

u64 timing_sync_get_hpet_frequency(void) {
    return timing_sync_info.hpet.frequency;
}

void timing_sync_enable_hpet(void) {
    if (!timing_sync_info.hpet_supported) return;
    
    volatile u64* hpet_base = (volatile u64*)timing_sync_info.hpet.base_address;
    u64 config = hpet_base[HPET_GENERAL_CONFIG >> 3];
    config |= HPET_CFG_ENABLE;
    hpet_base[HPET_GENERAL_CONFIG >> 3] = config;
}

void timing_sync_disable_hpet(void) {
    if (!timing_sync_info.hpet_supported) return;
    
    volatile u64* hpet_base = (volatile u64*)timing_sync_info.hpet.base_address;
    u64 config = hpet_base[HPET_GENERAL_CONFIG >> 3];
    config &= ~HPET_CFG_ENABLE;
    hpet_base[HPET_GENERAL_CONFIG >> 3] = config;
}

void timing_sync_setup_hpet_timer(u8 timer_id, u64 period, u8 periodic, u8 interrupt_vector) {
    if (!timing_sync_info.hpet_supported || timer_id >= timing_sync_info.hpet.num_timers) return;
    
    volatile u64* hpet_base = (volatile u64*)timing_sync_info.hpet.base_address;
    u32 timer_config_offset = HPET_TIMER0_CONFIG + (timer_id * 0x20);
    u32 timer_comp_offset = HPET_TIMER0_COMPARATOR + (timer_id * 0x20);
    
    u64 config = hpet_base[timer_config_offset >> 3];
    config &= ~0xFF; /* Clear interrupt vector */
    config |= interrupt_vector;
    
    if (periodic) {
        config |= HPET_TIMER_TYPE_PERIODIC | HPET_TIMER_VAL_SET;
    } else {
        config &= ~HPET_TIMER_TYPE_PERIODIC;
    }
    
    config |= HPET_TIMER_INT_ENABLE;
    
    hpet_base[timer_config_offset >> 3] = config;
    hpet_base[timer_comp_offset >> 3] = timing_sync_get_hpet_counter() + period;
    
    if (periodic) {
        hpet_base[timer_comp_offset >> 3] = period;
    }
}

void timing_sync_barrier_init(u32 num_cpus) {
    timing_sync_info.sync_barrier.count = 0;
    timing_sync_info.sync_barrier.sense = 0;
    timing_sync_info.sync_barrier.total_cpus = num_cpus;
}

void timing_sync_barrier_wait(void) {
    u64 start_time = timing_sync_get_tsc();
    
    u32 local_sense = timing_sync_info.sync_barrier.sense;
    
    /* Atomic increment */
    __asm__ volatile("lock incl %0" : "+m"(timing_sync_info.sync_barrier.count));
    
    if (timing_sync_info.sync_barrier.count == timing_sync_info.sync_barrier.total_cpus) {
        /* Last CPU to arrive */
        timing_sync_info.sync_barrier.count = 0;
        timing_sync_info.sync_barrier.sense = !local_sense;
    } else {
        /* Wait for barrier to complete */
        while (timing_sync_info.sync_barrier.sense == local_sense) {
            __asm__ volatile("pause");
        }
    }
    
    u64 end_time = timing_sync_get_tsc();
    u64 latency = end_time - start_time;
    
    timing_sync_info.sync_operations++;
    timing_sync_info.last_sync_time = end_time;
    
    if (latency > timing_sync_info.max_sync_latency) {
        timing_sync_info.max_sync_latency = latency;
    }
    
    if (latency < timing_sync_info.min_sync_latency) {
        timing_sync_info.min_sync_latency = latency;
    }
}

u64 timing_sync_measure_latency(void (*function)(void)) {
    if (!timing_sync_info.tsc_supported) return 0;
    
    u64 start = timing_sync_get_tsc();
    function();
    u64 end = timing_sync_get_tsc();
    
    timing_sync_info.timing_measurements++;
    return end - start;
}

u64 timing_sync_cycles_to_ns(u64 cycles) {
    if (timing_sync_info.tsc.frequency == 0) return 0;
    
    return (cycles * 1000000000ULL) / timing_sync_info.tsc.frequency;
}

u64 timing_sync_ns_to_cycles(u64 nanoseconds) {
    if (timing_sync_info.tsc.frequency == 0) return 0;
    
    return (nanoseconds * timing_sync_info.tsc.frequency) / 1000000000ULL;
}

void timing_sync_calibrate_tsc(void) {
    if (!timing_sync_info.tsc_supported || !timing_sync_info.hpet_supported) return;
    
    /* Use HPET to calibrate TSC frequency */
    u64 hpet_start = timing_sync_get_hpet_counter();
    u64 tsc_start = timing_sync_get_tsc();
    
    /* Wait for a known HPET period */
    u64 hpet_target = hpet_start + timing_sync_info.hpet.frequency; /* 1 second */
    while (timing_sync_get_hpet_counter() < hpet_target) {
        __asm__ volatile("pause");
    }
    
    u64 hpet_end = timing_sync_get_hpet_counter();
    u64 tsc_end = timing_sync_get_tsc();
    
    u64 hpet_elapsed = hpet_end - hpet_start;
    u64 tsc_elapsed = tsc_end - tsc_start;
    
    /* Calculate TSC frequency */
    timing_sync_info.tsc.frequency = (tsc_elapsed * timing_sync_info.hpet.frequency) / hpet_elapsed;
    timing_sync_info.tsc.reliable = 1;
}

u32 timing_sync_get_sync_operations(void) {
    return timing_sync_info.sync_operations;
}

u32 timing_sync_get_timing_measurements(void) {
    return timing_sync_info.timing_measurements;
}

u64 timing_sync_get_last_sync_time(void) {
    return timing_sync_info.last_sync_time;
}

u64 timing_sync_get_max_sync_latency(void) {
    return timing_sync_info.max_sync_latency;
}

u64 timing_sync_get_min_sync_latency(void) {
    return timing_sync_info.min_sync_latency;
}

u8 timing_sync_get_hpet_num_timers(void) {
    return timing_sync_info.hpet.num_timers;
}

u8 timing_sync_get_hpet_counter_size(void) {
    return timing_sync_info.hpet.counter_size;
}

void timing_sync_clear_statistics(void) {
    timing_sync_info.sync_operations = 0;
    timing_sync_info.timing_measurements = 0;
    timing_sync_info.max_sync_latency = 0;
    timing_sync_info.min_sync_latency = 0xFFFFFFFFFFFFFFFF;
    
    for (u8 i = 0; i < 8; i++) {
        timing_sync_info.hpet.timer_interrupts[i] = 0;
    }
}
