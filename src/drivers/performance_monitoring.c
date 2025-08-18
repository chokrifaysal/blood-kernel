/*
 * performance_monitoring.c – x86 CPU performance monitoring (PMC/PMU)
 */

#include "kernel/types.h"

/* Performance monitoring MSRs */
#define MSR_PERF_GLOBAL_CTRL            0x38F
#define MSR_PERF_GLOBAL_STATUS          0x38E
#define MSR_PERF_GLOBAL_OVF_CTRL        0x390
#define MSR_PERF_FIXED_CTR_CTRL         0x38D
#define MSR_PERF_CAPABILITIES           0x345

/* General purpose performance counters */
#define MSR_PERFEVTSEL0                 0x186
#define MSR_PERFEVTSEL1                 0x187
#define MSR_PERFEVTSEL2                 0x188
#define MSR_PERFEVTSEL3                 0x189
#define MSR_PMC0                        0xC1
#define MSR_PMC1                        0xC2
#define MSR_PMC2                        0xC3
#define MSR_PMC3                        0xC4

/* Fixed function performance counters */
#define MSR_FIXED_CTR0                  0x309  /* Instructions retired */
#define MSR_FIXED_CTR1                  0x30A  /* CPU cycles */
#define MSR_FIXED_CTR2                  0x30B  /* Reference cycles */

/* Performance event select bits */
#define PERFEVTSEL_EVENT_MASK           0xFF
#define PERFEVTSEL_UMASK_MASK           0xFF00
#define PERFEVTSEL_UMASK_SHIFT          8
#define PERFEVTSEL_USR                  (1 << 16)
#define PERFEVTSEL_OS                   (1 << 17)
#define PERFEVTSEL_EDGE                 (1 << 18)
#define PERFEVTSEL_PC                   (1 << 19)
#define PERFEVTSEL_INT                  (1 << 20)
#define PERFEVTSEL_ANY                  (1 << 21)
#define PERFEVTSEL_EN                   (1 << 22)
#define PERFEVTSEL_INV                  (1 << 23)
#define PERFEVTSEL_CMASK_MASK           0xFF000000
#define PERFEVTSEL_CMASK_SHIFT          24

/* Common performance events */
#define EVENT_UNHALTED_CORE_CYCLES      0x3C
#define EVENT_INSTRUCTION_RETIRED       0xC0
#define EVENT_UNHALTED_REF_CYCLES       0x3C
#define EVENT_LLC_REFERENCES            0x2E
#define EVENT_LLC_MISSES                0x2E
#define EVENT_BRANCH_INSTRUCTIONS       0xC4
#define EVENT_BRANCH_MISSES             0xC5
#define EVENT_CACHE_REFERENCES          0x2E
#define EVENT_CACHE_MISSES              0x2E
#define EVENT_BUS_CYCLES                0x3C
#define EVENT_STALLED_CYCLES_FRONTEND   0x9C
#define EVENT_STALLED_CYCLES_BACKEND    0xA1
#define EVENT_REF_CPU_CYCLES            0x3C

/* UMASK values for events */
#define UMASK_LLC_REFERENCES            0x4F
#define UMASK_LLC_MISSES                0x41
#define UMASK_L1D_REPLACEMENT           0x01
#define UMASK_L1I_REPLACEMENT           0x02
#define UMASK_DTLB_LOAD_MISSES          0x01
#define UMASK_ITLB_MISSES               0x01

typedef struct {
    u32 event_select;
    u32 umask;
    u8 user_mode;
    u8 kernel_mode;
    u8 edge_detect;
    u8 pin_control;
    u8 interrupt_enable;
    u8 any_thread;
    u8 enable;
    u8 invert;
    u32 counter_mask;
} perf_event_config_t;

typedef struct {
    u64 value;
    u64 last_value;
    u64 overflow_count;
    u8 enabled;
    perf_event_config_t config;
} perf_counter_t;

typedef struct {
    u8 performance_monitoring_supported;
    u8 architectural_perfmon_supported;
    u8 version;
    u8 num_gp_counters;
    u8 gp_counter_width;
    u8 num_fixed_counters;
    u8 fixed_counter_width;
    u8 anythread_deprecated;
    perf_counter_t gp_counters[8];
    perf_counter_t fixed_counters[3];
    u64 global_ctrl;
    u64 global_status;
    u64 global_ovf_ctrl;
    u64 fixed_ctrl;
    u32 total_samples;
    u32 overflow_interrupts;
    u64 last_sample_time;
    u64 monitoring_start_time;
    u64 total_monitoring_time;
} performance_monitoring_info_t;

static performance_monitoring_info_t perf_info;

extern u64 msr_read(u32 msr);
extern void msr_write(u32 msr, u64 value);
extern u8 msr_is_supported(void);
extern u64 timer_get_ticks(void);

static void performance_monitoring_detect_capabilities(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check for architectural performance monitoring */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0xA));
    
    if ((eax & 0xFF) > 0) {
        perf_info.architectural_perfmon_supported = 1;
        perf_info.version = eax & 0xFF;
        perf_info.num_gp_counters = (eax >> 8) & 0xFF;
        perf_info.gp_counter_width = (eax >> 16) & 0xFF;
        perf_info.num_fixed_counters = edx & 0x1F;
        perf_info.fixed_counter_width = (edx >> 5) & 0xFF;
        perf_info.anythread_deprecated = (edx >> 15) & 0x1;
        
        perf_info.performance_monitoring_supported = 1;
    }
}

void performance_monitoring_init(void) {
    perf_info.performance_monitoring_supported = 0;
    perf_info.architectural_perfmon_supported = 0;
    perf_info.version = 0;
    perf_info.num_gp_counters = 0;
    perf_info.gp_counter_width = 0;
    perf_info.num_fixed_counters = 0;
    perf_info.fixed_counter_width = 0;
    perf_info.anythread_deprecated = 0;
    perf_info.global_ctrl = 0;
    perf_info.global_status = 0;
    perf_info.global_ovf_ctrl = 0;
    perf_info.fixed_ctrl = 0;
    perf_info.total_samples = 0;
    perf_info.overflow_interrupts = 0;
    perf_info.last_sample_time = 0;
    perf_info.monitoring_start_time = 0;
    perf_info.total_monitoring_time = 0;
    
    for (u8 i = 0; i < 8; i++) {
        perf_info.gp_counters[i].value = 0;
        perf_info.gp_counters[i].last_value = 0;
        perf_info.gp_counters[i].overflow_count = 0;
        perf_info.gp_counters[i].enabled = 0;
        perf_info.gp_counters[i].config.event_select = 0;
        perf_info.gp_counters[i].config.umask = 0;
        perf_info.gp_counters[i].config.user_mode = 0;
        perf_info.gp_counters[i].config.kernel_mode = 0;
        perf_info.gp_counters[i].config.edge_detect = 0;
        perf_info.gp_counters[i].config.pin_control = 0;
        perf_info.gp_counters[i].config.interrupt_enable = 0;
        perf_info.gp_counters[i].config.any_thread = 0;
        perf_info.gp_counters[i].config.enable = 0;
        perf_info.gp_counters[i].config.invert = 0;
        perf_info.gp_counters[i].config.counter_mask = 0;
    }
    
    for (u8 i = 0; i < 3; i++) {
        perf_info.fixed_counters[i].value = 0;
        perf_info.fixed_counters[i].last_value = 0;
        perf_info.fixed_counters[i].overflow_count = 0;
        perf_info.fixed_counters[i].enabled = 0;
    }
    
    performance_monitoring_detect_capabilities();
    
    if (perf_info.performance_monitoring_supported) {
        perf_info.monitoring_start_time = timer_get_ticks();
    }
}

u8 performance_monitoring_is_supported(void) {
    return perf_info.performance_monitoring_supported;
}

u8 performance_monitoring_get_version(void) {
    return perf_info.version;
}

u8 performance_monitoring_get_num_gp_counters(void) {
    return perf_info.num_gp_counters;
}

u8 performance_monitoring_get_gp_counter_width(void) {
    return perf_info.gp_counter_width;
}

u8 performance_monitoring_get_num_fixed_counters(void) {
    return perf_info.num_fixed_counters;
}

u8 performance_monitoring_get_fixed_counter_width(void) {
    return perf_info.fixed_counter_width;
}

u8 performance_monitoring_setup_gp_counter(u8 counter, u32 event, u32 umask, u8 user, u8 kernel) {
    if (counter >= perf_info.num_gp_counters || !msr_is_supported()) return 0;
    
    perf_counter_t* pmc = &perf_info.gp_counters[counter];
    
    /* Configure event */
    pmc->config.event_select = event;
    pmc->config.umask = umask;
    pmc->config.user_mode = user;
    pmc->config.kernel_mode = kernel;
    pmc->config.enable = 1;
    
    /* Build event select value */
    u64 evtsel = event | (umask << PERFEVTSEL_UMASK_SHIFT);
    if (user) evtsel |= PERFEVTSEL_USR;
    if (kernel) evtsel |= PERFEVTSEL_OS;
    evtsel |= PERFEVTSEL_EN;
    
    /* Write to event select MSR */
    msr_write(MSR_PERFEVTSEL0 + counter, evtsel);
    
    /* Reset counter */
    msr_write(MSR_PMC0 + counter, 0);
    
    pmc->enabled = 1;
    pmc->value = 0;
    pmc->last_value = 0;
    
    return 1;
}

u8 performance_monitoring_enable_fixed_counter(u8 counter, u8 user, u8 kernel) {
    if (counter >= perf_info.num_fixed_counters || !msr_is_supported()) return 0;
    
    /* Configure fixed counter control */
    u64 fixed_ctrl = msr_read(MSR_PERF_FIXED_CTR_CTRL);
    u64 ctrl_bits = 0;
    
    if (kernel) ctrl_bits |= 0x1;
    if (user) ctrl_bits |= 0x2;
    
    fixed_ctrl &= ~(0xF << (counter * 4));
    fixed_ctrl |= (ctrl_bits << (counter * 4));
    
    msr_write(MSR_PERF_FIXED_CTR_CTRL, fixed_ctrl);
    
    /* Enable in global control */
    u64 global_ctrl = msr_read(MSR_PERF_GLOBAL_CTRL);
    global_ctrl |= (1ULL << (32 + counter));
    msr_write(MSR_PERF_GLOBAL_CTRL, global_ctrl);
    
    perf_info.fixed_counters[counter].enabled = 1;
    perf_info.fixed_ctrl = fixed_ctrl;
    perf_info.global_ctrl = global_ctrl;
    
    return 1;
}

u64 performance_monitoring_read_gp_counter(u8 counter) {
    if (counter >= perf_info.num_gp_counters || !msr_is_supported()) return 0;
    
    u64 value = msr_read(MSR_PMC0 + counter);
    perf_info.gp_counters[counter].last_value = perf_info.gp_counters[counter].value;
    perf_info.gp_counters[counter].value = value;
    
    return value;
}

u64 performance_monitoring_read_fixed_counter(u8 counter) {
    if (counter >= perf_info.num_fixed_counters || !msr_is_supported()) return 0;
    
    u64 value = msr_read(MSR_FIXED_CTR0 + counter);
    perf_info.fixed_counters[counter].last_value = perf_info.fixed_counters[counter].value;
    perf_info.fixed_counters[counter].value = value;
    
    return value;
}

void performance_monitoring_start_global(void) {
    if (!perf_info.performance_monitoring_supported || !msr_is_supported()) return;
    
    /* Enable all configured counters */
    u64 global_ctrl = 0;
    
    for (u8 i = 0; i < perf_info.num_gp_counters; i++) {
        if (perf_info.gp_counters[i].enabled) {
            global_ctrl |= (1ULL << i);
        }
    }
    
    for (u8 i = 0; i < perf_info.num_fixed_counters; i++) {
        if (perf_info.fixed_counters[i].enabled) {
            global_ctrl |= (1ULL << (32 + i));
        }
    }
    
    msr_write(MSR_PERF_GLOBAL_CTRL, global_ctrl);
    perf_info.global_ctrl = global_ctrl;
}

void performance_monitoring_stop_global(void) {
    if (!perf_info.performance_monitoring_supported || !msr_is_supported()) return;
    
    msr_write(MSR_PERF_GLOBAL_CTRL, 0);
    perf_info.global_ctrl = 0;
}

u64 performance_monitoring_get_instructions_retired(void) {
    return performance_monitoring_read_fixed_counter(0);
}

u64 performance_monitoring_get_cpu_cycles(void) {
    return performance_monitoring_read_fixed_counter(1);
}

u64 performance_monitoring_get_reference_cycles(void) {
    return performance_monitoring_read_fixed_counter(2);
}

u32 performance_monitoring_calculate_ipc(void) {
    u64 instructions = performance_monitoring_get_instructions_retired();
    u64 cycles = performance_monitoring_get_cpu_cycles();
    
    if (cycles == 0) return 0;
    
    return (u32)((instructions * 1000) / cycles);  /* IPC * 1000 */
}

void performance_monitoring_sample_all_counters(void) {
    if (!perf_info.performance_monitoring_supported) return;
    
    /* Read all enabled counters */
    for (u8 i = 0; i < perf_info.num_gp_counters; i++) {
        if (perf_info.gp_counters[i].enabled) {
            performance_monitoring_read_gp_counter(i);
        }
    }
    
    for (u8 i = 0; i < perf_info.num_fixed_counters; i++) {
        if (perf_info.fixed_counters[i].enabled) {
            performance_monitoring_read_fixed_counter(i);
        }
    }
    
    perf_info.total_samples++;
    perf_info.last_sample_time = timer_get_ticks();
    
    if (perf_info.monitoring_start_time > 0) {
        perf_info.total_monitoring_time = perf_info.last_sample_time - perf_info.monitoring_start_time;
    }
}

void performance_monitoring_handle_overflow(u8 counter) {
    if (counter < perf_info.num_gp_counters) {
        perf_info.gp_counters[counter].overflow_count++;
    } else if (counter >= 32 && (counter - 32) < perf_info.num_fixed_counters) {
        perf_info.fixed_counters[counter - 32].overflow_count++;
    }
    
    perf_info.overflow_interrupts++;
}

u64 performance_monitoring_get_global_status(void) {
    if (!msr_is_supported()) return 0;
    
    u64 status = msr_read(MSR_PERF_GLOBAL_STATUS);
    perf_info.global_status = status;
    return status;
}

void performance_monitoring_clear_global_status(void) {
    if (!msr_is_supported()) return;
    
    u64 status = performance_monitoring_get_global_status();
    msr_write(MSR_PERF_GLOBAL_OVF_CTRL, status);
    perf_info.global_ovf_ctrl = status;
}

const perf_counter_t* performance_monitoring_get_gp_counter_info(u8 counter) {
    if (counter >= perf_info.num_gp_counters) return 0;
    return &perf_info.gp_counters[counter];
}

const perf_counter_t* performance_monitoring_get_fixed_counter_info(u8 counter) {
    if (counter >= perf_info.num_fixed_counters) return 0;
    return &perf_info.fixed_counters[counter];
}

u32 performance_monitoring_get_total_samples(void) {
    return perf_info.total_samples;
}

u32 performance_monitoring_get_overflow_interrupts(void) {
    return perf_info.overflow_interrupts;
}

u64 performance_monitoring_get_last_sample_time(void) {
    return perf_info.last_sample_time;
}

u64 performance_monitoring_get_total_monitoring_time(void) {
    return perf_info.total_monitoring_time;
}

void performance_monitoring_clear_statistics(void) {
    perf_info.total_samples = 0;
    perf_info.overflow_interrupts = 0;
    
    for (u8 i = 0; i < perf_info.num_gp_counters; i++) {
        perf_info.gp_counters[i].overflow_count = 0;
    }
    
    for (u8 i = 0; i < perf_info.num_fixed_counters; i++) {
        perf_info.fixed_counters[i].overflow_count = 0;
    }
}
