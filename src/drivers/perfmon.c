/*
 * perfmon.c – x86 performance monitoring and profiling
 */

#include "kernel/types.h"

/* Performance monitoring MSRs */
#define MSR_PERF_GLOBAL_CTRL        0x38F
#define MSR_PERF_GLOBAL_STATUS      0x38E
#define MSR_PERF_GLOBAL_OVF_CTRL    0x390
#define MSR_PERF_CAPABILITIES       0x345
#define MSR_FIXED_CTR_CTRL          0x38D
#define MSR_FIXED_CTR0              0x309  /* Instructions retired */
#define MSR_FIXED_CTR1              0x30A  /* CPU cycles */
#define MSR_FIXED_CTR2              0x30B  /* Reference cycles */
#define MSR_PERFEVTSEL0             0x186
#define MSR_PERFEVTSEL1             0x187
#define MSR_PERFEVTSEL2             0x188
#define MSR_PERFEVTSEL3             0x189
#define MSR_PMC0                    0xC1
#define MSR_PMC1                    0xC2
#define MSR_PMC2                    0xC3
#define MSR_PMC3                    0xC4

/* Performance event select bits */
#define PERFEVTSEL_EVENT_SELECT     0x000000FF
#define PERFEVTSEL_UMASK            0x0000FF00
#define PERFEVTSEL_USR              0x00010000  /* User mode */
#define PERFEVTSEL_OS               0x00020000  /* Kernel mode */
#define PERFEVTSEL_EDGE             0x00040000  /* Edge detect */
#define PERFEVTSEL_PC               0x00080000  /* Pin control */
#define PERFEVTSEL_INT              0x00100000  /* APIC interrupt enable */
#define PERFEVTSEL_ANY              0x00200000  /* Any thread */
#define PERFEVTSEL_EN               0x00400000  /* Enable */
#define PERFEVTSEL_INV              0x00800000  /* Invert */
#define PERFEVTSEL_CMASK            0xFF000000  /* Counter mask */

/* Common performance events */
#define PERF_EVENT_CYCLES           0x3C  /* CPU cycles */
#define PERF_EVENT_INSTRUCTIONS     0xC0  /* Instructions retired */
#define PERF_EVENT_CACHE_REFERENCES 0x2E  /* Cache references */
#define PERF_EVENT_CACHE_MISSES     0x2E  /* Cache misses */
#define PERF_EVENT_BRANCH_INSTRUCTIONS 0xC4  /* Branch instructions */
#define PERF_EVENT_BRANCH_MISSES    0xC5  /* Branch mispredictions */
#define PERF_EVENT_BUS_CYCLES       0x3C  /* Bus cycles */
#define PERF_EVENT_STALLED_CYCLES_FRONTEND 0x9C  /* Frontend stalls */
#define PERF_EVENT_STALLED_CYCLES_BACKEND 0xA2   /* Backend stalls */
#define PERF_EVENT_REF_CYCLES       0x3C  /* Reference cycles */

/* UMASK values for cache events */
#define UMASK_CACHE_LL_REFS         0x4F  /* Last level cache references */
#define UMASK_CACHE_LL_MISSES       0x41  /* Last level cache misses */
#define UMASK_CACHE_L1D_REFS        0x01  /* L1 data cache references */
#define UMASK_CACHE_L1D_MISSES      0x08  /* L1 data cache misses */

typedef struct {
    u32 event;
    u32 umask;
    u8 user_mode;
    u8 kernel_mode;
    u8 enabled;
    u64 count;
    const char* name;
} perf_counter_t;

typedef struct {
    u8 supported;
    u8 version;
    u8 num_counters;
    u8 counter_width;
    u8 num_fixed_counters;
    u8 fixed_counter_width;
    u8 global_ctrl_supported;
    perf_counter_t counters[8];
    perf_counter_t fixed_counters[3];
    u64 global_ctrl;
    u64 global_status;
    u32 capabilities;
} perfmon_info_t;

static perfmon_info_t perfmon_info;

extern u64 msr_read(u32 msr);
extern void msr_write(u32 msr, u64 value);
extern u8 msr_is_supported(void);

static u8 perfmon_check_support(void) {
    u32 eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0xA));
    
    if ((eax & 0xFF) < 1) return 0;
    
    perfmon_info.version = eax & 0xFF;
    perfmon_info.num_counters = (eax >> 8) & 0xFF;
    perfmon_info.counter_width = (eax >> 16) & 0xFF;
    perfmon_info.num_fixed_counters = edx & 0x1F;
    perfmon_info.fixed_counter_width = (edx >> 5) & 0xFF;
    
    return 1;
}

static void perfmon_init_counters(void) {
    /* Initialize programmable counters */
    for (u8 i = 0; i < perfmon_info.num_counters && i < 8; i++) {
        perfmon_info.counters[i].event = 0;
        perfmon_info.counters[i].umask = 0;
        perfmon_info.counters[i].user_mode = 1;
        perfmon_info.counters[i].kernel_mode = 1;
        perfmon_info.counters[i].enabled = 0;
        perfmon_info.counters[i].count = 0;
        perfmon_info.counters[i].name = "Unused";
    }
    
    /* Initialize fixed counters */
    if (perfmon_info.num_fixed_counters >= 1) {
        perfmon_info.fixed_counters[0].name = "Instructions Retired";
        perfmon_info.fixed_counters[0].enabled = 0;
    }
    if (perfmon_info.num_fixed_counters >= 2) {
        perfmon_info.fixed_counters[1].name = "CPU Cycles";
        perfmon_info.fixed_counters[1].enabled = 0;
    }
    if (perfmon_info.num_fixed_counters >= 3) {
        perfmon_info.fixed_counters[2].name = "Reference Cycles";
        perfmon_info.fixed_counters[2].enabled = 0;
    }
}

void perfmon_init(void) {
    if (!msr_is_supported()) return;
    
    perfmon_info.supported = perfmon_check_support();
    if (!perfmon_info.supported) return;
    
    /* Check for global control support */
    perfmon_info.global_ctrl_supported = (perfmon_info.version >= 2);
    
    /* Read capabilities */
    if (perfmon_info.version >= 2) {
        perfmon_info.capabilities = msr_read(MSR_PERF_CAPABILITIES);
    }
    
    perfmon_init_counters();
    
    /* Disable all counters initially */
    perfmon_disable_all();
}

void perfmon_setup_counter(u8 counter, u32 event, u32 umask, u8 user_mode, u8 kernel_mode, const char* name) {
    if (!perfmon_info.supported || counter >= perfmon_info.num_counters || counter >= 8) {
        return;
    }
    
    perf_counter_t* cnt = &perfmon_info.counters[counter];
    cnt->event = event;
    cnt->umask = umask;
    cnt->user_mode = user_mode;
    cnt->kernel_mode = kernel_mode;
    cnt->name = name;
    cnt->enabled = 0;
    cnt->count = 0;
    
    /* Configure event select register */
    u64 evtsel = event | (umask << 8);
    if (user_mode) evtsel |= PERFEVTSEL_USR;
    if (kernel_mode) evtsel |= PERFEVTSEL_OS;
    
    msr_write(MSR_PERFEVTSEL0 + counter, evtsel);
    msr_write(MSR_PMC0 + counter, 0);
}

void perfmon_enable_counter(u8 counter) {
    if (!perfmon_info.supported || counter >= perfmon_info.num_counters || counter >= 8) {
        return;
    }
    
    perf_counter_t* cnt = &perfmon_info.counters[counter];
    cnt->enabled = 1;
    
    /* Enable counter in event select register */
    u64 evtsel = msr_read(MSR_PERFEVTSEL0 + counter);
    evtsel |= PERFEVTSEL_EN;
    msr_write(MSR_PERFEVTSEL0 + counter, evtsel);
    
    /* Enable in global control if supported */
    if (perfmon_info.global_ctrl_supported) {
        perfmon_info.global_ctrl |= (1ULL << counter);
        msr_write(MSR_PERF_GLOBAL_CTRL, perfmon_info.global_ctrl);
    }
}

void perfmon_disable_counter(u8 counter) {
    if (!perfmon_info.supported || counter >= perfmon_info.num_counters || counter >= 8) {
        return;
    }
    
    perf_counter_t* cnt = &perfmon_info.counters[counter];
    cnt->enabled = 0;
    
    /* Disable counter in event select register */
    u64 evtsel = msr_read(MSR_PERFEVTSEL0 + counter);
    evtsel &= ~PERFEVTSEL_EN;
    msr_write(MSR_PERFEVTSEL0 + counter, evtsel);
    
    /* Disable in global control if supported */
    if (perfmon_info.global_ctrl_supported) {
        perfmon_info.global_ctrl &= ~(1ULL << counter);
        msr_write(MSR_PERF_GLOBAL_CTRL, perfmon_info.global_ctrl);
    }
}

u64 perfmon_read_counter(u8 counter) {
    if (!perfmon_info.supported || counter >= perfmon_info.num_counters || counter >= 8) {
        return 0;
    }
    
    u64 count = msr_read(MSR_PMC0 + counter);
    perfmon_info.counters[counter].count = count;
    return count;
}

void perfmon_reset_counter(u8 counter) {
    if (!perfmon_info.supported || counter >= perfmon_info.num_counters || counter >= 8) {
        return;
    }
    
    msr_write(MSR_PMC0 + counter, 0);
    perfmon_info.counters[counter].count = 0;
}

void perfmon_enable_fixed_counter(u8 counter) {
    if (!perfmon_info.supported || counter >= perfmon_info.num_fixed_counters || counter >= 3) {
        return;
    }
    
    perfmon_info.fixed_counters[counter].enabled = 1;
    
    /* Configure fixed counter control */
    u64 fixed_ctrl = msr_read(MSR_FIXED_CTR_CTRL);
    u64 enable_bits = 0x3; /* Enable for user and kernel mode */
    fixed_ctrl |= (enable_bits << (counter * 4));
    msr_write(MSR_FIXED_CTR_CTRL, fixed_ctrl);
    
    /* Enable in global control if supported */
    if (perfmon_info.global_ctrl_supported) {
        perfmon_info.global_ctrl |= (1ULL << (32 + counter));
        msr_write(MSR_PERF_GLOBAL_CTRL, perfmon_info.global_ctrl);
    }
}

void perfmon_disable_fixed_counter(u8 counter) {
    if (!perfmon_info.supported || counter >= perfmon_info.num_fixed_counters || counter >= 3) {
        return;
    }
    
    perfmon_info.fixed_counters[counter].enabled = 0;
    
    /* Configure fixed counter control */
    u64 fixed_ctrl = msr_read(MSR_FIXED_CTR_CTRL);
    u64 disable_mask = ~(0xFULL << (counter * 4));
    fixed_ctrl &= disable_mask;
    msr_write(MSR_FIXED_CTR_CTRL, fixed_ctrl);
    
    /* Disable in global control if supported */
    if (perfmon_info.global_ctrl_supported) {
        perfmon_info.global_ctrl &= ~(1ULL << (32 + counter));
        msr_write(MSR_PERF_GLOBAL_CTRL, perfmon_info.global_ctrl);
    }
}

u64 perfmon_read_fixed_counter(u8 counter) {
    if (!perfmon_info.supported || counter >= perfmon_info.num_fixed_counters || counter >= 3) {
        return 0;
    }
    
    u64 count = msr_read(MSR_FIXED_CTR0 + counter);
    perfmon_info.fixed_counters[counter].count = count;
    return count;
}

void perfmon_reset_fixed_counter(u8 counter) {
    if (!perfmon_info.supported || counter >= perfmon_info.num_fixed_counters || counter >= 3) {
        return;
    }
    
    msr_write(MSR_FIXED_CTR0 + counter, 0);
    perfmon_info.fixed_counters[counter].count = 0;
}

void perfmon_enable_all(void) {
    if (!perfmon_info.supported) return;
    
    if (perfmon_info.global_ctrl_supported) {
        /* Enable all configured counters */
        u64 enable_mask = 0;
        for (u8 i = 0; i < perfmon_info.num_counters && i < 8; i++) {
            if (perfmon_info.counters[i].enabled) {
                enable_mask |= (1ULL << i);
            }
        }
        for (u8 i = 0; i < perfmon_info.num_fixed_counters && i < 3; i++) {
            if (perfmon_info.fixed_counters[i].enabled) {
                enable_mask |= (1ULL << (32 + i));
            }
        }
        
        perfmon_info.global_ctrl = enable_mask;
        msr_write(MSR_PERF_GLOBAL_CTRL, enable_mask);
    }
}

void perfmon_disable_all(void) {
    if (!perfmon_info.supported) return;
    
    if (perfmon_info.global_ctrl_supported) {
        msr_write(MSR_PERF_GLOBAL_CTRL, 0);
        perfmon_info.global_ctrl = 0;
    }
    
    /* Disable individual counters */
    for (u8 i = 0; i < perfmon_info.num_counters && i < 8; i++) {
        msr_write(MSR_PERFEVTSEL0 + i, 0);
        perfmon_info.counters[i].enabled = 0;
    }
    
    /* Disable fixed counters */
    msr_write(MSR_FIXED_CTR_CTRL, 0);
    for (u8 i = 0; i < perfmon_info.num_fixed_counters && i < 3; i++) {
        perfmon_info.fixed_counters[i].enabled = 0;
    }
}

u8 perfmon_is_supported(void) {
    return perfmon_info.supported;
}

u8 perfmon_get_version(void) {
    return perfmon_info.version;
}

u8 perfmon_get_num_counters(void) {
    return perfmon_info.num_counters;
}

u8 perfmon_get_counter_width(void) {
    return perfmon_info.counter_width;
}

u8 perfmon_get_num_fixed_counters(void) {
    return perfmon_info.num_fixed_counters;
}

const char* perfmon_get_counter_name(u8 counter) {
    if (counter >= perfmon_info.num_counters || counter >= 8) {
        return "Invalid";
    }
    return perfmon_info.counters[counter].name;
}

const char* perfmon_get_fixed_counter_name(u8 counter) {
    if (counter >= perfmon_info.num_fixed_counters || counter >= 3) {
        return "Invalid";
    }
    return perfmon_info.fixed_counters[counter].name;
}

u8 perfmon_is_counter_enabled(u8 counter) {
    if (counter >= perfmon_info.num_counters || counter >= 8) {
        return 0;
    }
    return perfmon_info.counters[counter].enabled;
}

u8 perfmon_is_fixed_counter_enabled(u8 counter) {
    if (counter >= perfmon_info.num_fixed_counters || counter >= 3) {
        return 0;
    }
    return perfmon_info.fixed_counters[counter].enabled;
}
