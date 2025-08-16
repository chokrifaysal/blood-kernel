/*
 * cpufreq.c – x86 CPU frequency scaling (SpeedStep/Cool'n'Quiet)
 */

#include "kernel/types.h"

/* Intel Enhanced SpeedStep MSRs */
#define MSR_IA32_PERF_STATUS    0x198
#define MSR_IA32_PERF_CTL       0x199
#define MSR_IA32_MISC_ENABLE    0x1A0
#define MSR_IA32_PLATFORM_INFO  0xCE
#define MSR_TURBO_RATIO_LIMIT   0x1AD

/* AMD Cool'n'Quiet MSRs */
#define MSR_PSTATE_CURRENT_LIMIT 0xC0010061
#define MSR_PSTATE_CONTROL      0xC0010062
#define MSR_PSTATE_STATUS       0xC0010063
#define MSR_PSTATE_0            0xC0010064
#define MSR_PSTATE_1            0xC0010065
#define MSR_PSTATE_2            0xC0010066
#define MSR_PSTATE_3            0xC0010067
#define MSR_PSTATE_4            0xC0010068
#define MSR_PSTATE_5            0xC0010069
#define MSR_PSTATE_6            0xC001006A
#define MSR_PSTATE_7            0xC001006B

/* MISC_ENABLE bits */
#define MISC_ENABLE_ENHANCED_SPEEDSTEP 0x10000
#define MISC_ENABLE_TURBO_DISABLE      0x4000000000ULL

/* P-state control bits */
#define PERF_CTL_TURBO_DISABLE  0x100000000ULL

typedef struct {
    u16 frequency;      /* MHz */
    u8 voltage;         /* Encoded voltage */
    u8 power;          /* TDP in watts */
    u8 latency;        /* Transition latency in us */
} pstate_info_t;

typedef struct {
    u8 cpufreq_supported;
    u8 intel_speedstep;
    u8 amd_coolnquiet;
    u8 turbo_supported;
    u8 turbo_enabled;
    u8 num_pstates;
    u8 current_pstate;
    u8 min_pstate;
    u8 max_pstate;
    u16 base_frequency;
    u16 max_turbo_frequency;
    u16 current_frequency;
    pstate_info_t pstates[16];
    u32 transition_latency;
    u8 governor_policy;
} cpufreq_info_t;

static cpufreq_info_t cpufreq_info;

/* Governor policies */
#define CPUFREQ_POLICY_PERFORMANCE  0
#define CPUFREQ_POLICY_POWERSAVE     1
#define CPUFREQ_POLICY_ONDEMAND      2
#define CPUFREQ_POLICY_CONSERVATIVE  3
#define CPUFREQ_POLICY_USERSPACE     4

extern u64 msr_read(u32 msr);
extern void msr_write(u32 msr, u64 value);
extern u8 msr_is_supported(void);

static u8 cpufreq_check_intel_speedstep(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check for Enhanced SpeedStep */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    
    if (!(ecx & (1 << 7))) return 0; /* No Enhanced SpeedStep */
    
    /* Check if SpeedStep is enabled in MISC_ENABLE */
    u64 misc_enable = msr_read(MSR_IA32_MISC_ENABLE);
    if (!(misc_enable & MISC_ENABLE_ENHANCED_SPEEDSTEP)) {
        /* Enable Enhanced SpeedStep */
        misc_enable |= MISC_ENABLE_ENHANCED_SPEEDSTEP;
        msr_write(MSR_IA32_MISC_ENABLE, misc_enable);
    }
    
    return 1;
}

static u8 cpufreq_check_amd_coolnquiet(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check vendor */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0));
    if (!(ebx == 0x68747541 && edx == 0x69746E65 && ecx == 0x444D4163)) return 0;
    
    /* Check for PowerNow! */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x80000007));
    
    return (edx & (1 << 1)) != 0; /* CPB (Core Performance Boost) */
}

static void cpufreq_detect_intel_pstates(void) {
    u64 platform_info = msr_read(MSR_IA32_PLATFORM_INFO);
    
    /* Extract frequency information */
    cpufreq_info.min_pstate = (platform_info >> 40) & 0xFF;
    cpufreq_info.max_pstate = (platform_info >> 8) & 0xFF;
    cpufreq_info.base_frequency = ((platform_info >> 8) & 0xFF) * 100; /* MHz */
    
    /* Check for turbo */
    u64 misc_enable = msr_read(MSR_IA32_MISC_ENABLE);
    cpufreq_info.turbo_supported = !(misc_enable & MISC_ENABLE_TURBO_DISABLE);
    
    if (cpufreq_info.turbo_supported) {
        u64 turbo_limit = msr_read(MSR_TURBO_RATIO_LIMIT);
        cpufreq_info.max_turbo_frequency = (turbo_limit & 0xFF) * 100; /* MHz */
    } else {
        cpufreq_info.max_turbo_frequency = cpufreq_info.base_frequency;
    }
    
    /* Build P-state table */
    cpufreq_info.num_pstates = 0;
    for (u8 ratio = cpufreq_info.min_pstate; ratio <= cpufreq_info.max_pstate; ratio++) {
        if (cpufreq_info.num_pstates >= 16) break;
        
        pstate_info_t* pstate = &cpufreq_info.pstates[cpufreq_info.num_pstates];
        pstate->frequency = ratio * 100;
        pstate->voltage = 100 - (ratio - cpufreq_info.min_pstate) * 2; /* Estimated */
        pstate->power = 15 + (ratio - cpufreq_info.min_pstate) * 5; /* Estimated TDP */
        pstate->latency = 10; /* 10us typical */
        
        cpufreq_info.num_pstates++;
    }
    
    cpufreq_info.transition_latency = 10;
}

static void cpufreq_detect_amd_pstates(void) {
    /* Read P-state limit */
    u64 pstate_limit = msr_read(MSR_PSTATE_CURRENT_LIMIT);
    u8 max_pstate = pstate_limit & 0x7;
    
    cpufreq_info.num_pstates = 0;
    cpufreq_info.min_pstate = 7;
    cpufreq_info.max_pstate = 0;
    
    /* Read available P-states */
    for (u8 i = 0; i <= max_pstate && i < 8; i++) {
        u64 pstate_msr = msr_read(MSR_PSTATE_0 + i);
        
        if (!(pstate_msr & (1ULL << 63))) continue; /* P-state not enabled */
        
        pstate_info_t* pstate = &cpufreq_info.pstates[cpufreq_info.num_pstates];
        
        /* Extract frequency (FID and DID) */
        u16 fid = (pstate_msr >> 0) & 0x3F;
        u16 did = (pstate_msr >> 6) & 0x7;
        pstate->frequency = (fid + 16) * 100 / (1 << did);
        
        /* Extract voltage (VID) */
        u16 vid = (pstate_msr >> 9) & 0x7F;
        pstate->voltage = 155 - vid; /* Simplified voltage calculation */
        
        pstate->power = 15 + i * 10; /* Estimated */
        pstate->latency = 100; /* 100us typical for AMD */
        
        if (i < cpufreq_info.min_pstate) cpufreq_info.min_pstate = i;
        if (i > cpufreq_info.max_pstate) cpufreq_info.max_pstate = i;
        
        cpufreq_info.num_pstates++;
    }
    
    if (cpufreq_info.num_pstates > 0) {
        cpufreq_info.base_frequency = cpufreq_info.pstates[0].frequency;
        cpufreq_info.max_turbo_frequency = cpufreq_info.pstates[cpufreq_info.num_pstates - 1].frequency;
    }
    
    cpufreq_info.transition_latency = 100;
}

void cpufreq_init(void) {
    cpufreq_info.cpufreq_supported = 0;
    cpufreq_info.intel_speedstep = 0;
    cpufreq_info.amd_coolnquiet = 0;
    cpufreq_info.turbo_supported = 0;
    cpufreq_info.turbo_enabled = 0;
    cpufreq_info.num_pstates = 0;
    cpufreq_info.current_pstate = 0;
    cpufreq_info.governor_policy = CPUFREQ_POLICY_ONDEMAND;
    
    if (!msr_is_supported()) return;
    
    /* Check for Intel Enhanced SpeedStep */
    if (cpufreq_check_intel_speedstep()) {
        cpufreq_info.intel_speedstep = 1;
        cpufreq_info.cpufreq_supported = 1;
        cpufreq_detect_intel_pstates();
    }
    /* Check for AMD Cool'n'Quiet */
    else if (cpufreq_check_amd_coolnquiet()) {
        cpufreq_info.amd_coolnquiet = 1;
        cpufreq_info.cpufreq_supported = 1;
        cpufreq_detect_amd_pstates();
    }
    
    if (cpufreq_info.cpufreq_supported) {
        /* Read current frequency */
        cpufreq_info.current_frequency = cpufreq_get_current_frequency();
        cpufreq_info.turbo_enabled = cpufreq_is_turbo_enabled();
    }
}

u8 cpufreq_is_supported(void) {
    return cpufreq_info.cpufreq_supported;
}

u8 cpufreq_is_intel_speedstep(void) {
    return cpufreq_info.intel_speedstep;
}

u8 cpufreq_is_amd_coolnquiet(void) {
    return cpufreq_info.amd_coolnquiet;
}

u8 cpufreq_is_turbo_supported(void) {
    return cpufreq_info.turbo_supported;
}

u8 cpufreq_is_turbo_enabled(void) {
    if (!cpufreq_info.cpufreq_supported) return 0;
    
    if (cpufreq_info.intel_speedstep) {
        u64 perf_ctl = msr_read(MSR_IA32_PERF_CTL);
        return !(perf_ctl & PERF_CTL_TURBO_DISABLE);
    } else if (cpufreq_info.amd_coolnquiet) {
        /* AMD turbo is controlled differently */
        return cpufreq_info.turbo_supported;
    }
    
    return 0;
}

void cpufreq_enable_turbo(void) {
    if (!cpufreq_info.turbo_supported) return;
    
    if (cpufreq_info.intel_speedstep) {
        u64 perf_ctl = msr_read(MSR_IA32_PERF_CTL);
        perf_ctl &= ~PERF_CTL_TURBO_DISABLE;
        msr_write(MSR_IA32_PERF_CTL, perf_ctl);
        cpufreq_info.turbo_enabled = 1;
    }
}

void cpufreq_disable_turbo(void) {
    if (!cpufreq_info.turbo_supported) return;
    
    if (cpufreq_info.intel_speedstep) {
        u64 perf_ctl = msr_read(MSR_IA32_PERF_CTL);
        perf_ctl |= PERF_CTL_TURBO_DISABLE;
        msr_write(MSR_IA32_PERF_CTL, perf_ctl);
        cpufreq_info.turbo_enabled = 0;
    }
}

u8 cpufreq_set_pstate(u8 pstate) {
    if (!cpufreq_info.cpufreq_supported || pstate >= cpufreq_info.num_pstates) return 0;
    
    if (cpufreq_info.intel_speedstep) {
        u8 ratio = cpufreq_info.min_pstate + pstate;
        u64 perf_ctl = msr_read(MSR_IA32_PERF_CTL);
        perf_ctl = (perf_ctl & ~0xFFFFULL) | ratio;
        msr_write(MSR_IA32_PERF_CTL, perf_ctl);
    } else if (cpufreq_info.amd_coolnquiet) {
        msr_write(MSR_PSTATE_CONTROL, pstate);
    }
    
    cpufreq_info.current_pstate = pstate;
    return 1;
}

u8 cpufreq_get_current_pstate(void) {
    if (!cpufreq_info.cpufreq_supported) return 0;
    
    if (cpufreq_info.intel_speedstep) {
        u64 perf_status = msr_read(MSR_IA32_PERF_STATUS);
        u8 ratio = (perf_status >> 8) & 0xFF;
        return ratio - cpufreq_info.min_pstate;
    } else if (cpufreq_info.amd_coolnquiet) {
        u64 pstate_status = msr_read(MSR_PSTATE_STATUS);
        return pstate_status & 0x7;
    }
    
    return 0;
}

u16 cpufreq_get_current_frequency(void) {
    if (!cpufreq_info.cpufreq_supported) return 0;
    
    u8 pstate = cpufreq_get_current_pstate();
    if (pstate < cpufreq_info.num_pstates) {
        return cpufreq_info.pstates[pstate].frequency;
    }
    
    return 0;
}

u8 cpufreq_get_num_pstates(void) {
    return cpufreq_info.num_pstates;
}

u16 cpufreq_get_pstate_frequency(u8 pstate) {
    if (pstate >= cpufreq_info.num_pstates) return 0;
    return cpufreq_info.pstates[pstate].frequency;
}

u8 cpufreq_get_pstate_voltage(u8 pstate) {
    if (pstate >= cpufreq_info.num_pstates) return 0;
    return cpufreq_info.pstates[pstate].voltage;
}

u8 cpufreq_get_pstate_power(u8 pstate) {
    if (pstate >= cpufreq_info.num_pstates) return 0;
    return cpufreq_info.pstates[pstate].power;
}

u16 cpufreq_get_base_frequency(void) {
    return cpufreq_info.base_frequency;
}

u16 cpufreq_get_max_turbo_frequency(void) {
    return cpufreq_info.max_turbo_frequency;
}

u32 cpufreq_get_transition_latency(void) {
    return cpufreq_info.transition_latency;
}

void cpufreq_set_governor_policy(u8 policy) {
    cpufreq_info.governor_policy = policy;
}

u8 cpufreq_get_governor_policy(void) {
    return cpufreq_info.governor_policy;
}

void cpufreq_set_frequency(u16 target_freq) {
    if (!cpufreq_info.cpufreq_supported) return;
    
    /* Find closest P-state */
    u8 best_pstate = 0;
    u16 best_diff = 0xFFFF;
    
    for (u8 i = 0; i < cpufreq_info.num_pstates; i++) {
        u16 freq = cpufreq_info.pstates[i].frequency;
        u16 diff = (freq > target_freq) ? (freq - target_freq) : (target_freq - freq);
        
        if (diff < best_diff) {
            best_diff = diff;
            best_pstate = i;
        }
    }
    
    cpufreq_set_pstate(best_pstate);
}
