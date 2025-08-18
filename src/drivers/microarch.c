/*
 * microarch.c – x86 CPU microarchitecture control (branch prediction/speculation)
 */

#include "kernel/types.h"

/* Speculation control MSRs */
#define MSR_SPEC_CTRL                   0x48
#define MSR_PRED_CMD                    0x49
#define MSR_ARCH_CAPABILITIES           0x10A

/* SPEC_CTRL bits */
#define SPEC_CTRL_IBRS                  (1 << 0)
#define SPEC_CTRL_STIBP                 (1 << 1)
#define SPEC_CTRL_SSBD                  (1 << 2)

/* PRED_CMD bits */
#define PRED_CMD_IBPB                   (1 << 0)

/* ARCH_CAPABILITIES bits */
#define ARCH_CAP_RDCL_NO                (1 << 0)
#define ARCH_CAP_IBRS_ALL               (1 << 1)
#define ARCH_CAP_RSBA                   (1 << 2)
#define ARCH_CAP_SKIP_L1DFL_VMENTRY     (1 << 3)
#define ARCH_CAP_SSB_NO                 (1 << 4)
#define ARCH_CAP_MDS_NO                 (1 << 5)
#define ARCH_CAP_IF_PSCHANGE_MC_NO      (1 << 6)
#define ARCH_CAP_TSX_CTRL               (1 << 7)
#define ARCH_CAP_TAA_NO                 (1 << 8)

/* Branch prediction MSRs */
#define MSR_LBR_SELECT                  0x1C8
#define MSR_LBR_TOS                     0x1C9
#define MSR_LASTBRANCH_0_FROM_IP        0x680
#define MSR_LASTBRANCH_0_TO_IP          0x6C0

/* Performance monitoring for branch prediction */
#define MSR_PERF_GLOBAL_CTRL            0x38F
#define MSR_PERFEVTSEL0                 0x186
#define MSR_PMC0                        0xC1

/* Branch prediction events */
#define EVENT_BRANCH_INSTRUCTIONS       0xC4
#define EVENT_BRANCH_MISSES             0xC5
#define EVENT_INDIRECT_BRANCHES         0xE4
#define EVENT_INDIRECT_BRANCH_MISSES    0xE6

typedef struct {
    u64 total_branches;
    u64 branch_misses;
    u64 indirect_branches;
    u64 indirect_misses;
    u32 misprediction_rate;
    u64 last_measurement;
} branch_stats_t;

typedef struct {
    u8 microarch_supported;
    u8 spec_ctrl_supported;
    u8 ibrs_supported;
    u8 stibp_supported;
    u8 ssbd_supported;
    u8 ibpb_supported;
    u8 lbr_supported;
    u8 ibrs_enabled;
    u8 stibp_enabled;
    u8 ssbd_enabled;
    u8 branch_prediction_enabled;
    u64 arch_capabilities;
    u64 spec_ctrl_value;
    branch_stats_t branch_stats;
    u32 speculation_mitigations;
    u32 branch_flushes;
    u64 last_ibpb_time;
    u8 lbr_depth;
    u64 lbr_entries[32];
} microarch_info_t;

static microarch_info_t microarch_info;

extern u64 msr_read(u32 msr);
extern void msr_write(u32 msr, u64 value);
extern u8 msr_is_supported(void);
extern u64 timer_get_ticks(void);

static void microarch_detect_capabilities(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check for speculation control support */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    
    microarch_info.spec_ctrl_supported = (edx & (1 << 26)) != 0;
    microarch_info.ibrs_supported = (edx & (1 << 26)) != 0;
    microarch_info.stibp_supported = (edx & (1 << 27)) != 0;
    microarch_info.ssbd_supported = (edx & (1 << 31)) != 0;
    microarch_info.ibpb_supported = (edx & (1 << 26)) != 0;
    
    /* Check for LBR support */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    microarch_info.lbr_supported = (edx & (1 << 21)) != 0;
    
    if (microarch_info.spec_ctrl_supported || microarch_info.lbr_supported) {
        microarch_info.microarch_supported = 1;
    }
    
    /* Read architectural capabilities */
    if (microarch_info.spec_ctrl_supported && msr_is_supported()) {
        microarch_info.arch_capabilities = msr_read(MSR_ARCH_CAPABILITIES);
    }
}

static void microarch_setup_branch_monitoring(void) {
    if (!microarch_info.lbr_supported || !msr_is_supported()) return;
    
    /* Enable LBR */
    u64 debugctl = 0;
    debugctl |= (1 << 0); /* LBR enable */
    debugctl |= (1 << 6); /* Freeze LBR on PMI */
    
    /* Write to IA32_DEBUGCTL MSR */
    msr_write(0x1D9, debugctl);
    
    /* Setup performance counters for branch monitoring */
    u64 evtsel = EVENT_BRANCH_INSTRUCTIONS | (1 << 16) | (1 << 17) | (1 << 22);
    msr_write(MSR_PERFEVTSEL0, evtsel);
    
    /* Enable global performance monitoring */
    u64 global_ctrl = msr_read(MSR_PERF_GLOBAL_CTRL);
    global_ctrl |= (1 << 0);
    msr_write(MSR_PERF_GLOBAL_CTRL, global_ctrl);
    
    microarch_info.branch_prediction_enabled = 1;
    microarch_info.lbr_depth = 16;
}

void microarch_init(void) {
    microarch_info.microarch_supported = 0;
    microarch_info.spec_ctrl_supported = 0;
    microarch_info.ibrs_supported = 0;
    microarch_info.stibp_supported = 0;
    microarch_info.ssbd_supported = 0;
    microarch_info.ibpb_supported = 0;
    microarch_info.lbr_supported = 0;
    microarch_info.ibrs_enabled = 0;
    microarch_info.stibp_enabled = 0;
    microarch_info.ssbd_enabled = 0;
    microarch_info.branch_prediction_enabled = 0;
    microarch_info.arch_capabilities = 0;
    microarch_info.spec_ctrl_value = 0;
    microarch_info.speculation_mitigations = 0;
    microarch_info.branch_flushes = 0;
    microarch_info.last_ibpb_time = 0;
    microarch_info.lbr_depth = 0;
    
    /* Initialize branch statistics */
    microarch_info.branch_stats.total_branches = 0;
    microarch_info.branch_stats.branch_misses = 0;
    microarch_info.branch_stats.indirect_branches = 0;
    microarch_info.branch_stats.indirect_misses = 0;
    microarch_info.branch_stats.misprediction_rate = 0;
    microarch_info.branch_stats.last_measurement = 0;
    
    for (u8 i = 0; i < 32; i++) {
        microarch_info.lbr_entries[i] = 0;
    }
    
    microarch_detect_capabilities();
    
    if (microarch_info.microarch_supported) {
        microarch_setup_branch_monitoring();
    }
}

u8 microarch_is_supported(void) {
    return microarch_info.microarch_supported;
}

u8 microarch_is_spec_ctrl_supported(void) {
    return microarch_info.spec_ctrl_supported;
}

u8 microarch_is_ibrs_supported(void) {
    return microarch_info.ibrs_supported;
}

u8 microarch_is_stibp_supported(void) {
    return microarch_info.stibp_supported;
}

u8 microarch_is_ssbd_supported(void) {
    return microarch_info.ssbd_supported;
}

void microarch_enable_ibrs(void) {
    if (!microarch_info.ibrs_supported || !msr_is_supported()) return;
    
    u64 spec_ctrl = msr_read(MSR_SPEC_CTRL);
    spec_ctrl |= SPEC_CTRL_IBRS;
    msr_write(MSR_SPEC_CTRL, spec_ctrl);
    
    microarch_info.spec_ctrl_value = spec_ctrl;
    microarch_info.ibrs_enabled = 1;
    microarch_info.speculation_mitigations++;
}

void microarch_disable_ibrs(void) {
    if (!microarch_info.ibrs_supported || !msr_is_supported()) return;
    
    u64 spec_ctrl = msr_read(MSR_SPEC_CTRL);
    spec_ctrl &= ~SPEC_CTRL_IBRS;
    msr_write(MSR_SPEC_CTRL, spec_ctrl);
    
    microarch_info.spec_ctrl_value = spec_ctrl;
    microarch_info.ibrs_enabled = 0;
}

void microarch_enable_stibp(void) {
    if (!microarch_info.stibp_supported || !msr_is_supported()) return;
    
    u64 spec_ctrl = msr_read(MSR_SPEC_CTRL);
    spec_ctrl |= SPEC_CTRL_STIBP;
    msr_write(MSR_SPEC_CTRL, spec_ctrl);
    
    microarch_info.spec_ctrl_value = spec_ctrl;
    microarch_info.stibp_enabled = 1;
    microarch_info.speculation_mitigations++;
}

void microarch_disable_stibp(void) {
    if (!microarch_info.stibp_supported || !msr_is_supported()) return;
    
    u64 spec_ctrl = msr_read(MSR_SPEC_CTRL);
    spec_ctrl &= ~SPEC_CTRL_STIBP;
    msr_write(MSR_SPEC_CTRL, spec_ctrl);
    
    microarch_info.spec_ctrl_value = spec_ctrl;
    microarch_info.stibp_enabled = 0;
}

void microarch_enable_ssbd(void) {
    if (!microarch_info.ssbd_supported || !msr_is_supported()) return;
    
    u64 spec_ctrl = msr_read(MSR_SPEC_CTRL);
    spec_ctrl |= SPEC_CTRL_SSBD;
    msr_write(MSR_SPEC_CTRL, spec_ctrl);
    
    microarch_info.spec_ctrl_value = spec_ctrl;
    microarch_info.ssbd_enabled = 1;
    microarch_info.speculation_mitigations++;
}

void microarch_disable_ssbd(void) {
    if (!microarch_info.ssbd_supported || !msr_is_supported()) return;
    
    u64 spec_ctrl = msr_read(MSR_SPEC_CTRL);
    spec_ctrl &= ~SPEC_CTRL_SSBD;
    msr_write(MSR_SPEC_CTRL, spec_ctrl);
    
    microarch_info.spec_ctrl_value = spec_ctrl;
    microarch_info.ssbd_enabled = 0;
}

void microarch_flush_indirect_branches(void) {
    if (!microarch_info.ibpb_supported || !msr_is_supported()) return;
    
    msr_write(MSR_PRED_CMD, PRED_CMD_IBPB);
    microarch_info.branch_flushes++;
    microarch_info.last_ibpb_time = timer_get_ticks();
}

u8 microarch_is_ibrs_enabled(void) {
    return microarch_info.ibrs_enabled;
}

u8 microarch_is_stibp_enabled(void) {
    return microarch_info.stibp_enabled;
}

u8 microarch_is_ssbd_enabled(void) {
    return microarch_info.ssbd_enabled;
}

u64 microarch_get_arch_capabilities(void) {
    return microarch_info.arch_capabilities;
}

u64 microarch_get_spec_ctrl_value(void) {
    return microarch_info.spec_ctrl_value;
}

void microarch_update_branch_stats(void) {
    if (!microarch_info.branch_prediction_enabled || !msr_is_supported()) return;
    
    u64 current_time = timer_get_ticks();
    
    /* Read branch instruction counter */
    u64 branches = msr_read(MSR_PMC0);
    
    /* Setup counter for branch misses */
    u64 evtsel = EVENT_BRANCH_MISSES | (1 << 16) | (1 << 17) | (1 << 22);
    msr_write(MSR_PERFEVTSEL0, evtsel);
    
    /* Small delay to accumulate some data */
    for (volatile u32 i = 0; i < 10000; i++);
    
    u64 misses = msr_read(MSR_PMC0);
    
    microarch_info.branch_stats.total_branches = branches;
    microarch_info.branch_stats.branch_misses = misses;
    
    if (branches > 0) {
        microarch_info.branch_stats.misprediction_rate = (u32)((misses * 100) / branches);
    }
    
    microarch_info.branch_stats.last_measurement = current_time;
    
    /* Reset counter for next measurement */
    msr_write(MSR_PMC0, 0);
}

const branch_stats_t* microarch_get_branch_stats(void) {
    return &microarch_info.branch_stats;
}

u32 microarch_get_speculation_mitigations(void) {
    return microarch_info.speculation_mitigations;
}

u32 microarch_get_branch_flushes(void) {
    return microarch_info.branch_flushes;
}

u64 microarch_get_last_ibpb_time(void) {
    return microarch_info.last_ibpb_time;
}

void microarch_read_lbr_stack(void) {
    if (!microarch_info.lbr_supported || !msr_is_supported()) return;
    
    u8 tos = msr_read(MSR_LBR_TOS) & 0xF;
    
    for (u8 i = 0; i < microarch_info.lbr_depth && i < 32; i++) {
        u8 index = (tos - i) & 0xF;
        microarch_info.lbr_entries[i] = msr_read(MSR_LASTBRANCH_0_FROM_IP + index);
    }
}

const u64* microarch_get_lbr_entries(void) {
    return microarch_info.lbr_entries;
}

u8 microarch_get_lbr_depth(void) {
    return microarch_info.lbr_depth;
}

u8 microarch_check_vulnerability(u32 vulnerability) {
    u64 caps = microarch_info.arch_capabilities;
    
    switch (vulnerability) {
        case 0: /* Meltdown */
            return (caps & ARCH_CAP_RDCL_NO) == 0;
        case 1: /* Spectre v2 */
            return (caps & ARCH_CAP_IBRS_ALL) == 0;
        case 2: /* SSB */
            return (caps & ARCH_CAP_SSB_NO) == 0;
        case 3: /* MDS */
            return (caps & ARCH_CAP_MDS_NO) == 0;
        case 4: /* TAA */
            return (caps & ARCH_CAP_TAA_NO) == 0;
        default:
            return 0;
    }
}

void microarch_enable_all_mitigations(void) {
    if (microarch_info.ibrs_supported) microarch_enable_ibrs();
    if (microarch_info.stibp_supported) microarch_enable_stibp();
    if (microarch_info.ssbd_supported) microarch_enable_ssbd();
}

void microarch_disable_all_mitigations(void) {
    if (microarch_info.ibrs_supported) microarch_disable_ibrs();
    if (microarch_info.stibp_supported) microarch_disable_stibp();
    if (microarch_info.ssbd_supported) microarch_disable_ssbd();
}

u8 microarch_is_branch_prediction_enabled(void) {
    return microarch_info.branch_prediction_enabled;
}

void microarch_clear_statistics(void) {
    microarch_info.branch_stats.total_branches = 0;
    microarch_info.branch_stats.branch_misses = 0;
    microarch_info.branch_stats.indirect_branches = 0;
    microarch_info.branch_stats.indirect_misses = 0;
    microarch_info.branch_stats.misprediction_rate = 0;
    microarch_info.speculation_mitigations = 0;
    microarch_info.branch_flushes = 0;
}
