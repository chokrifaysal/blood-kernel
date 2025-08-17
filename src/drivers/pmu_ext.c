/*
 * pmu_ext.c – x86 advanced performance monitoring unit extensions
 */

#include "kernel/types.h"

/* PMU MSRs */
#define MSR_PERF_GLOBAL_STATUS      0x38E
#define MSR_PERF_GLOBAL_CTRL        0x38F
#define MSR_PERF_GLOBAL_OVF_CTRL    0x390
#define MSR_PERF_GLOBAL_STATUS_RESET 0x390
#define MSR_PERF_GLOBAL_STATUS_SET  0x391
#define MSR_PERF_GLOBAL_INUSE       0x392

/* Fixed-function PMCs */
#define MSR_PERF_FIXED_CTR0         0x309
#define MSR_PERF_FIXED_CTR1         0x30A
#define MSR_PERF_FIXED_CTR2         0x30B
#define MSR_PERF_FIXED_CTR_CTRL     0x38D

/* General-purpose PMCs */
#define MSR_PERFEVTSEL0             0x186
#define MSR_PERFEVTSEL1             0x187
#define MSR_PERFEVTSEL2             0x188
#define MSR_PERFEVTSEL3             0x189
#define MSR_PMC0                    0xC1
#define MSR_PMC1                    0xC2
#define MSR_PMC2                    0xC3
#define MSR_PMC3                    0xC4

/* PEBS MSRs */
#define MSR_PEBS_ENABLE             0x3F1
#define MSR_DS_AREA                 0x600

/* LBR MSRs */
#define MSR_LBR_SELECT              0x1C8
#define MSR_LBR_TOS                 0x1C9
#define MSR_LASTBRANCH_0_FROM_IP    0x680
#define MSR_LASTBRANCH_0_TO_IP      0x6C0

/* Event select bits */
#define PERFEVTSEL_EVENT_MASK       0xFF
#define PERFEVTSEL_UMASK_MASK       0xFF00
#define PERFEVTSEL_USR              (1 << 16)
#define PERFEVTSEL_OS               (1 << 17)
#define PERFEVTSEL_EDGE             (1 << 18)
#define PERFEVTSEL_PC               (1 << 19)
#define PERFEVTSEL_INT              (1 << 20)
#define PERFEVTSEL_ANY              (1 << 21)
#define PERFEVTSEL_EN               (1 << 22)
#define PERFEVTSEL_INV              (1 << 23)
#define PERFEVTSEL_CMASK_MASK       0xFF000000

/* Common performance events */
#define PMU_EVENT_CYCLES            0x3C
#define PMU_EVENT_INSTRUCTIONS      0xC0
#define PMU_EVENT_CACHE_REFERENCES  0x2E
#define PMU_EVENT_CACHE_MISSES      0x2E
#define PMU_EVENT_BRANCH_INSTRUCTIONS 0xC4
#define PMU_EVENT_BRANCH_MISSES     0xC5
#define PMU_EVENT_BUS_CYCLES        0x3C
#define PMU_EVENT_STALLED_CYCLES_FRONTEND 0x9C
#define PMU_EVENT_STALLED_CYCLES_BACKEND  0xA1

typedef struct {
    u32 event;
    u32 umask;
    u8 user_mode;
    u8 kernel_mode;
    u8 edge_detect;
    u8 pin_control;
    u8 interrupt_enable;
    u8 any_thread;
    u8 invert;
    u8 counter_mask;
    u64 count;
    u64 sample_period;
    u32 overflow_count;
} pmu_counter_t;

typedef struct {
    u64 from_ip;
    u64 to_ip;
    u64 info;
} lbr_entry_t;

typedef struct {
    u8 pmu_ext_supported;
    u8 version;
    u8 num_gp_counters;
    u8 gp_counter_width;
    u8 num_fixed_counters;
    u8 fixed_counter_width;
    u8 pebs_supported;
    u8 lbr_supported;
    u8 freeze_on_pmi;
    u8 freeze_on_overflow;
    pmu_counter_t gp_counters[8];
    pmu_counter_t fixed_counters[3];
    u8 lbr_depth;
    lbr_entry_t lbr_stack[32];
    u8 lbr_tos;
    u64 global_status;
    u64 global_ctrl;
    u32 pmi_count;
    u32 overflow_count;
} pmu_ext_info_t;

static pmu_ext_info_t pmu_ext_info;

extern u64 msr_read(u32 msr);
extern void msr_write(u32 msr, u64 value);
extern u8 msr_is_supported(void);

static void pmu_ext_detect_capabilities(void) {
    u32 eax, ebx, ecx, edx;
    
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0xA));
    
    pmu_ext_info.version = eax & 0xFF;
    pmu_ext_info.num_gp_counters = (eax >> 8) & 0xFF;
    pmu_ext_info.gp_counter_width = (eax >> 16) & 0xFF;
    pmu_ext_info.num_fixed_counters = edx & 0x1F;
    pmu_ext_info.fixed_counter_width = (edx >> 5) & 0xFF;
    
    if (pmu_ext_info.version >= 1) {
        pmu_ext_info.pmu_ext_supported = 1;
    }
    
    if (pmu_ext_info.num_gp_counters > 8) {
        pmu_ext_info.num_gp_counters = 8;
    }
    
    if (pmu_ext_info.num_fixed_counters > 3) {
        pmu_ext_info.num_fixed_counters = 3;
    }
    
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    pmu_ext_info.pebs_supported = (ebx & (1 << 4)) != 0;
    
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    pmu_ext_info.lbr_supported = (edx & (1 << 21)) != 0;
    
    if (pmu_ext_info.lbr_supported) {
        pmu_ext_info.lbr_depth = 16;
    }
}

void pmu_ext_init(void) {
    pmu_ext_info.pmu_ext_supported = 0;
    pmu_ext_info.version = 0;
    pmu_ext_info.num_gp_counters = 0;
    pmu_ext_info.gp_counter_width = 0;
    pmu_ext_info.num_fixed_counters = 0;
    pmu_ext_info.fixed_counter_width = 0;
    pmu_ext_info.pebs_supported = 0;
    pmu_ext_info.lbr_supported = 0;
    pmu_ext_info.freeze_on_pmi = 0;
    pmu_ext_info.freeze_on_overflow = 0;
    pmu_ext_info.lbr_depth = 0;
    pmu_ext_info.lbr_tos = 0;
    pmu_ext_info.global_status = 0;
    pmu_ext_info.global_ctrl = 0;
    pmu_ext_info.pmi_count = 0;
    pmu_ext_info.overflow_count = 0;
    
    for (u8 i = 0; i < 8; i++) {
        pmu_ext_info.gp_counters[i].event = 0;
        pmu_ext_info.gp_counters[i].count = 0;
        pmu_ext_info.gp_counters[i].overflow_count = 0;
    }
    
    for (u8 i = 0; i < 3; i++) {
        pmu_ext_info.fixed_counters[i].event = 0;
        pmu_ext_info.fixed_counters[i].count = 0;
        pmu_ext_info.fixed_counters[i].overflow_count = 0;
    }
    
    pmu_ext_detect_capabilities();
    
    if (pmu_ext_info.pmu_ext_supported && msr_is_supported()) {
        msr_write(MSR_PERF_GLOBAL_CTRL, 0);
        msr_write(MSR_PERF_GLOBAL_OVF_CTRL, 0);
    }
}

u8 pmu_ext_is_supported(void) {
    return pmu_ext_info.pmu_ext_supported;
}

u8 pmu_ext_get_version(void) {
    return pmu_ext_info.version;
}

u8 pmu_ext_get_num_gp_counters(void) {
    return pmu_ext_info.num_gp_counters;
}

u8 pmu_ext_get_num_fixed_counters(void) {
    return pmu_ext_info.num_fixed_counters;
}

u8 pmu_ext_is_pebs_supported(void) {
    return pmu_ext_info.pebs_supported;
}

u8 pmu_ext_is_lbr_supported(void) {
    return pmu_ext_info.lbr_supported;
}

u8 pmu_ext_setup_gp_counter(u8 index, u32 event, u32 umask, u8 user, u8 kernel) {
    if (!pmu_ext_info.pmu_ext_supported || !msr_is_supported()) return 0;
    if (index >= pmu_ext_info.num_gp_counters) return 0;
    
    pmu_counter_t* counter = &pmu_ext_info.gp_counters[index];
    
    counter->event = event;
    counter->umask = umask;
    counter->user_mode = user;
    counter->kernel_mode = kernel;
    counter->edge_detect = 0;
    counter->pin_control = 0;
    counter->interrupt_enable = 0;
    counter->any_thread = 0;
    counter->invert = 0;
    counter->counter_mask = 0;
    counter->count = 0;
    counter->overflow_count = 0;
    
    u64 evtsel = event | (umask << 8);
    if (user) evtsel |= PERFEVTSEL_USR;
    if (kernel) evtsel |= PERFEVTSEL_OS;
    evtsel |= PERFEVTSEL_EN;
    
    msr_write(MSR_PERFEVTSEL0 + index, evtsel);
    msr_write(MSR_PMC0 + index, 0);
    
    return 1;
}

u8 pmu_ext_enable_counter(u8 index) {
    if (!pmu_ext_info.pmu_ext_supported || !msr_is_supported()) return 0;
    if (index >= pmu_ext_info.num_gp_counters) return 0;
    
    u64 global_ctrl = msr_read(MSR_PERF_GLOBAL_CTRL);
    global_ctrl |= (1ULL << index);
    msr_write(MSR_PERF_GLOBAL_CTRL, global_ctrl);
    
    pmu_ext_info.global_ctrl = global_ctrl;
    return 1;
}

u8 pmu_ext_disable_counter(u8 index) {
    if (!pmu_ext_info.pmu_ext_supported || !msr_is_supported()) return 0;
    if (index >= pmu_ext_info.num_gp_counters) return 0;
    
    u64 global_ctrl = msr_read(MSR_PERF_GLOBAL_CTRL);
    global_ctrl &= ~(1ULL << index);
    msr_write(MSR_PERF_GLOBAL_CTRL, global_ctrl);
    
    pmu_ext_info.global_ctrl = global_ctrl;
    return 1;
}

u64 pmu_ext_read_counter(u8 index) {
    if (!pmu_ext_info.pmu_ext_supported || !msr_is_supported()) return 0;
    if (index >= pmu_ext_info.num_gp_counters) return 0;
    
    u64 count = msr_read(MSR_PMC0 + index);
    pmu_ext_info.gp_counters[index].count = count;
    return count;
}

void pmu_ext_reset_counter(u8 index) {
    if (!pmu_ext_info.pmu_ext_supported || !msr_is_supported()) return;
    if (index >= pmu_ext_info.num_gp_counters) return;
    
    msr_write(MSR_PMC0 + index, 0);
    pmu_ext_info.gp_counters[index].count = 0;
    pmu_ext_info.gp_counters[index].overflow_count = 0;
}

u8 pmu_ext_setup_fixed_counter(u8 index, u8 user, u8 kernel, u8 enable_pmi) {
    if (!pmu_ext_info.pmu_ext_supported || !msr_is_supported()) return 0;
    if (index >= pmu_ext_info.num_fixed_counters) return 0;
    
    u64 fixed_ctrl = msr_read(MSR_PERF_FIXED_CTR_CTRL);
    
    u32 shift = index * 4;
    fixed_ctrl &= ~(0xF << shift);
    
    if (kernel) fixed_ctrl |= (1 << shift);
    if (user) fixed_ctrl |= (2 << shift);
    if (enable_pmi) fixed_ctrl |= (8 << shift);
    
    msr_write(MSR_PERF_FIXED_CTR_CTRL, fixed_ctrl);
    
    u64 global_ctrl = msr_read(MSR_PERF_GLOBAL_CTRL);
    global_ctrl |= (1ULL << (32 + index));
    msr_write(MSR_PERF_GLOBAL_CTRL, global_ctrl);
    
    pmu_ext_info.global_ctrl = global_ctrl;
    return 1;
}

u64 pmu_ext_read_fixed_counter(u8 index) {
    if (!pmu_ext_info.pmu_ext_supported || !msr_is_supported()) return 0;
    if (index >= pmu_ext_info.num_fixed_counters) return 0;
    
    u64 count = msr_read(MSR_PERF_FIXED_CTR0 + index);
    pmu_ext_info.fixed_counters[index].count = count;
    return count;
}

void pmu_ext_enable_lbr(void) {
    if (!pmu_ext_info.lbr_supported || !msr_is_supported()) return;
    
    msr_write(MSR_LBR_SELECT, 0);
    
    u64 debugctl = 0;
    debugctl |= (1 << 0);
    debugctl |= (1 << 6);
    
    extern void msr_write_debugctl(u64 value);
    msr_write_debugctl(debugctl);
}

void pmu_ext_disable_lbr(void) {
    if (!pmu_ext_info.lbr_supported || !msr_is_supported()) return;
    
    extern void msr_write_debugctl(u64 value);
    msr_write_debugctl(0);
}

void pmu_ext_read_lbr_stack(void) {
    if (!pmu_ext_info.lbr_supported || !msr_is_supported()) return;
    
    pmu_ext_info.lbr_tos = msr_read(MSR_LBR_TOS) & 0xF;
    
    for (u8 i = 0; i < pmu_ext_info.lbr_depth && i < 32; i++) {
        pmu_ext_info.lbr_stack[i].from_ip = msr_read(MSR_LASTBRANCH_0_FROM_IP + i);
        pmu_ext_info.lbr_stack[i].to_ip = msr_read(MSR_LASTBRANCH_0_TO_IP + i);
        pmu_ext_info.lbr_stack[i].info = 0;
    }
}

const lbr_entry_t* pmu_ext_get_lbr_entry(u8 index) {
    if (index >= pmu_ext_info.lbr_depth || index >= 32) return 0;
    return &pmu_ext_info.lbr_stack[index];
}

u8 pmu_ext_get_lbr_tos(void) {
    return pmu_ext_info.lbr_tos;
}

u8 pmu_ext_get_lbr_depth(void) {
    return pmu_ext_info.lbr_depth;
}

void pmu_ext_handle_pmi(void) {
    if (!pmu_ext_info.pmu_ext_supported || !msr_is_supported()) return;
    
    pmu_ext_info.pmi_count++;
    
    u64 global_status = msr_read(MSR_PERF_GLOBAL_STATUS);
    pmu_ext_info.global_status = global_status;
    
    for (u8 i = 0; i < pmu_ext_info.num_gp_counters; i++) {
        if (global_status & (1ULL << i)) {
            pmu_ext_info.gp_counters[i].overflow_count++;
            pmu_ext_info.overflow_count++;
        }
    }
    
    for (u8 i = 0; i < pmu_ext_info.num_fixed_counters; i++) {
        if (global_status & (1ULL << (32 + i))) {
            pmu_ext_info.fixed_counters[i].overflow_count++;
            pmu_ext_info.overflow_count++;
        }
    }
    
    msr_write(MSR_PERF_GLOBAL_OVF_CTRL, global_status);
}

u32 pmu_ext_get_pmi_count(void) {
    return pmu_ext_info.pmi_count;
}

u32 pmu_ext_get_overflow_count(void) {
    return pmu_ext_info.overflow_count;
}

u64 pmu_ext_get_global_status(void) {
    if (!pmu_ext_info.pmu_ext_supported || !msr_is_supported()) return 0;
    
    pmu_ext_info.global_status = msr_read(MSR_PERF_GLOBAL_STATUS);
    return pmu_ext_info.global_status;
}

const pmu_counter_t* pmu_ext_get_gp_counter_info(u8 index) {
    if (index >= pmu_ext_info.num_gp_counters) return 0;
    return &pmu_ext_info.gp_counters[index];
}

const pmu_counter_t* pmu_ext_get_fixed_counter_info(u8 index) {
    if (index >= pmu_ext_info.num_fixed_counters) return 0;
    return &pmu_ext_info.fixed_counters[index];
}
