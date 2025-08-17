/*
 * freq_scaling.c – x86 CPU frequency scaling (P-states/Turbo)
 */

#include "kernel/types.h"

/* P-state MSRs */
#define MSR_PERF_CTL                0x199
#define MSR_PERF_STATUS             0x198
#define MSR_PLATFORM_INFO           0xCE
#define MSR_TURBO_RATIO_LIMIT       0x1AD
#define MSR_TURBO_RATIO_LIMIT1      0x1AE
#define MSR_TURBO_RATIO_LIMIT2      0x1AF

/* Intel SpeedStep MSRs */
#define MSR_IA32_MISC_ENABLE        0x1A0
#define MSR_IA32_ENERGY_PERF_BIAS   0x1B0

/* AMD Cool'n'Quiet MSRs */
#define MSR_PSTATE_CURRENT_LIMIT    0xC0010061
#define MSR_PSTATE_CONTROL          0xC0010062
#define MSR_PSTATE_STATUS           0xC0010063
#define MSR_PSTATE_0                0xC0010064
#define MSR_PSTATE_1                0xC0010065
#define MSR_PSTATE_2                0xC0010066
#define MSR_PSTATE_3                0xC0010067
#define MSR_PSTATE_4                0xC0010068
#define MSR_PSTATE_5                0xC0010069
#define MSR_PSTATE_6                0xC001006A
#define MSR_PSTATE_7                0xC001006B

/* PERF_CTL bits */
#define PERF_CTL_TURBO_DISABLE      (1ULL << 32)

/* MISC_ENABLE bits */
#define MISC_ENABLE_SPEEDSTEP       (1ULL << 16)
#define MISC_ENABLE_TURBO_DISABLE   (1ULL << 38)

/* Energy Performance Bias */
#define EPB_PERFORMANCE             0
#define EPB_BALANCED_PERFORMANCE    4
#define EPB_BALANCED_POWER          8
#define EPB_POWER_SAVE              15

typedef struct {
    u16 frequency_mhz;
    u8 ratio;
    u8 voltage;
    u32 power_mw;
    u8 enabled;
    u32 transition_latency_us;
} pstate_info_t;

typedef struct {
    u8 freq_scaling_supported;
    u8 speedstep_supported;
    u8 turbo_supported;
    u8 eist_supported;
    u8 amd_pstate_supported;
    u8 turbo_enabled;
    u8 speedstep_enabled;
    u8 current_pstate;
    u8 max_pstate;
    u8 min_pstate;
    u8 turbo_pstate;
    u16 base_frequency_mhz;
    u16 max_frequency_mhz;
    u16 current_frequency_mhz;
    u8 num_pstates;
    pstate_info_t pstates[16];
    u8 energy_perf_bias;
    u32 frequency_transitions;
    u32 turbo_activations;
    u64 last_transition_time;
    u8 thermal_throttling;
    u8 power_limit_throttling;
} freq_scaling_info_t;

static freq_scaling_info_t freq_scaling_info;

extern u64 msr_read(u32 msr);
extern void msr_write(u32 msr, u64 value);
extern u8 msr_is_supported(void);
extern u64 timer_get_ticks(void);

static void freq_scaling_detect_capabilities(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check for Intel EIST (Enhanced Intel SpeedStep) */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    freq_scaling_info.eist_supported = (ecx & (1 << 7)) != 0;
    
    /* Check for Intel Turbo Boost */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(6));
    freq_scaling_info.turbo_supported = (eax & (1 << 1)) != 0;
    
    /* Check for AMD PowerNow! */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x80000001));
    freq_scaling_info.amd_pstate_supported = (edx & (1 << 7)) != 0;
    
    if (freq_scaling_info.eist_supported || freq_scaling_info.amd_pstate_supported) {
        freq_scaling_info.freq_scaling_supported = 1;
        freq_scaling_info.speedstep_supported = freq_scaling_info.eist_supported;
    }
}

static void freq_scaling_read_intel_pstates(void) {
    if (!freq_scaling_info.eist_supported || !msr_is_supported()) return;
    
    u64 platform_info = msr_read(MSR_PLATFORM_INFO);
    freq_scaling_info.max_pstate = (platform_info >> 8) & 0xFF;
    freq_scaling_info.min_pstate = (platform_info >> 40) & 0xFF;
    
    freq_scaling_info.base_frequency_mhz = freq_scaling_info.max_pstate * 100;
    
    if (freq_scaling_info.turbo_supported) {
        u64 turbo_limit = msr_read(MSR_TURBO_RATIO_LIMIT);
        freq_scaling_info.turbo_pstate = turbo_limit & 0xFF;
        freq_scaling_info.max_frequency_mhz = freq_scaling_info.turbo_pstate * 100;
    } else {
        freq_scaling_info.max_frequency_mhz = freq_scaling_info.base_frequency_mhz;
    }
    
    /* Enumerate P-states */
    u8 pstate_count = 0;
    for (u8 ratio = freq_scaling_info.min_pstate; ratio <= freq_scaling_info.max_pstate && pstate_count < 16; ratio++) {
        freq_scaling_info.pstates[pstate_count].ratio = ratio;
        freq_scaling_info.pstates[pstate_count].frequency_mhz = ratio * 100;
        freq_scaling_info.pstates[pstate_count].enabled = 1;
        freq_scaling_info.pstates[pstate_count].transition_latency_us = 10;
        freq_scaling_info.pstates[pstate_count].power_mw = 1000 + (ratio * 50);
        pstate_count++;
    }
    
    if (freq_scaling_info.turbo_supported && pstate_count < 16) {
        freq_scaling_info.pstates[pstate_count].ratio = freq_scaling_info.turbo_pstate;
        freq_scaling_info.pstates[pstate_count].frequency_mhz = freq_scaling_info.turbo_pstate * 100;
        freq_scaling_info.pstates[pstate_count].enabled = 1;
        freq_scaling_info.pstates[pstate_count].transition_latency_us = 50;
        freq_scaling_info.pstates[pstate_count].power_mw = 2000;
        pstate_count++;
    }
    
    freq_scaling_info.num_pstates = pstate_count;
}

static void freq_scaling_read_amd_pstates(void) {
    if (!freq_scaling_info.amd_pstate_supported || !msr_is_supported()) return;
    
    u8 pstate_count = 0;
    for (u8 i = 0; i < 8 && pstate_count < 16; i++) {
        u64 pstate_msr = msr_read(MSR_PSTATE_0 + i);
        
        if (!(pstate_msr & (1ULL << 63))) continue; /* P-state not enabled */
        
        u16 fid = pstate_msr & 0x3F;
        u16 did = (pstate_msr >> 6) & 0x7;
        u16 frequency = (fid + 16) * 100 / (1 << did);
        
        freq_scaling_info.pstates[pstate_count].ratio = i;
        freq_scaling_info.pstates[pstate_count].frequency_mhz = frequency;
        freq_scaling_info.pstates[pstate_count].enabled = 1;
        freq_scaling_info.pstates[pstate_count].transition_latency_us = 100;
        freq_scaling_info.pstates[pstate_count].power_mw = 1000 + (frequency * 2);
        pstate_count++;
    }
    
    freq_scaling_info.num_pstates = pstate_count;
    if (pstate_count > 0) {
        freq_scaling_info.max_frequency_mhz = freq_scaling_info.pstates[0].frequency_mhz;
        freq_scaling_info.base_frequency_mhz = freq_scaling_info.pstates[pstate_count - 1].frequency_mhz;
    }
}

void freq_scaling_init(void) {
    freq_scaling_info.freq_scaling_supported = 0;
    freq_scaling_info.speedstep_supported = 0;
    freq_scaling_info.turbo_supported = 0;
    freq_scaling_info.eist_supported = 0;
    freq_scaling_info.amd_pstate_supported = 0;
    freq_scaling_info.turbo_enabled = 0;
    freq_scaling_info.speedstep_enabled = 0;
    freq_scaling_info.current_pstate = 0;
    freq_scaling_info.max_pstate = 0;
    freq_scaling_info.min_pstate = 0;
    freq_scaling_info.turbo_pstate = 0;
    freq_scaling_info.base_frequency_mhz = 0;
    freq_scaling_info.max_frequency_mhz = 0;
    freq_scaling_info.current_frequency_mhz = 0;
    freq_scaling_info.num_pstates = 0;
    freq_scaling_info.energy_perf_bias = EPB_BALANCED_PERFORMANCE;
    freq_scaling_info.frequency_transitions = 0;
    freq_scaling_info.turbo_activations = 0;
    freq_scaling_info.last_transition_time = 0;
    freq_scaling_info.thermal_throttling = 0;
    freq_scaling_info.power_limit_throttling = 0;
    
    for (u8 i = 0; i < 16; i++) {
        freq_scaling_info.pstates[i].frequency_mhz = 0;
        freq_scaling_info.pstates[i].ratio = 0;
        freq_scaling_info.pstates[i].voltage = 0;
        freq_scaling_info.pstates[i].power_mw = 0;
        freq_scaling_info.pstates[i].enabled = 0;
        freq_scaling_info.pstates[i].transition_latency_us = 0;
    }
    
    freq_scaling_detect_capabilities();
    
    if (freq_scaling_info.freq_scaling_supported) {
        if (freq_scaling_info.eist_supported) {
            freq_scaling_read_intel_pstates();
        } else if (freq_scaling_info.amd_pstate_supported) {
            freq_scaling_read_amd_pstates();
        }
        
        freq_scaling_info.current_frequency_mhz = freq_scaling_get_current_frequency();
    }
}

u8 freq_scaling_is_supported(void) {
    return freq_scaling_info.freq_scaling_supported;
}

u8 freq_scaling_is_turbo_supported(void) {
    return freq_scaling_info.turbo_supported;
}

u8 freq_scaling_is_speedstep_supported(void) {
    return freq_scaling_info.speedstep_supported;
}

u16 freq_scaling_get_current_frequency(void) {
    if (!freq_scaling_info.freq_scaling_supported || !msr_is_supported()) return 0;
    
    if (freq_scaling_info.eist_supported) {
        u64 perf_status = msr_read(MSR_PERF_STATUS);
        u8 ratio = (perf_status >> 8) & 0xFF;
        freq_scaling_info.current_frequency_mhz = ratio * 100;
    } else if (freq_scaling_info.amd_pstate_supported) {
        u64 pstate_status = msr_read(MSR_PSTATE_STATUS);
        u8 current_pstate = pstate_status & 0x7;
        if (current_pstate < freq_scaling_info.num_pstates) {
            freq_scaling_info.current_frequency_mhz = freq_scaling_info.pstates[current_pstate].frequency_mhz;
        }
    }
    
    return freq_scaling_info.current_frequency_mhz;
}

u8 freq_scaling_set_frequency(u16 frequency_mhz) {
    if (!freq_scaling_info.freq_scaling_supported || !msr_is_supported()) return 0;
    
    /* Find closest P-state */
    u8 target_pstate = 0;
    u16 closest_freq = 0xFFFF;
    
    for (u8 i = 0; i < freq_scaling_info.num_pstates; i++) {
        if (!freq_scaling_info.pstates[i].enabled) continue;
        
        u16 freq_diff = (frequency_mhz > freq_scaling_info.pstates[i].frequency_mhz) ?
                        frequency_mhz - freq_scaling_info.pstates[i].frequency_mhz :
                        freq_scaling_info.pstates[i].frequency_mhz - frequency_mhz;
        
        if (freq_diff < closest_freq) {
            closest_freq = freq_diff;
            target_pstate = i;
        }
    }
    
    if (freq_scaling_info.eist_supported) {
        u64 perf_ctl = (u64)freq_scaling_info.pstates[target_pstate].ratio << 8;
        msr_write(MSR_PERF_CTL, perf_ctl);
    } else if (freq_scaling_info.amd_pstate_supported) {
        msr_write(MSR_PSTATE_CONTROL, target_pstate);
    }
    
    freq_scaling_info.current_pstate = target_pstate;
    freq_scaling_info.frequency_transitions++;
    freq_scaling_info.last_transition_time = timer_get_ticks();
    
    return 1;
}

u8 freq_scaling_set_pstate(u8 pstate) {
    if (!freq_scaling_info.freq_scaling_supported || pstate >= freq_scaling_info.num_pstates) return 0;
    if (!freq_scaling_info.pstates[pstate].enabled) return 0;
    
    return freq_scaling_set_frequency(freq_scaling_info.pstates[pstate].frequency_mhz);
}

void freq_scaling_enable_turbo(void) {
    if (!freq_scaling_info.turbo_supported || !msr_is_supported()) return;
    
    u64 misc_enable = msr_read(MSR_IA32_MISC_ENABLE);
    misc_enable &= ~MISC_ENABLE_TURBO_DISABLE;
    msr_write(MSR_IA32_MISC_ENABLE, misc_enable);
    
    freq_scaling_info.turbo_enabled = 1;
}

void freq_scaling_disable_turbo(void) {
    if (!freq_scaling_info.turbo_supported || !msr_is_supported()) return;
    
    u64 misc_enable = msr_read(MSR_IA32_MISC_ENABLE);
    misc_enable |= MISC_ENABLE_TURBO_DISABLE;
    msr_write(MSR_IA32_MISC_ENABLE, misc_enable);
    
    freq_scaling_info.turbo_enabled = 0;
}

u8 freq_scaling_is_turbo_enabled(void) {
    if (!freq_scaling_info.turbo_supported || !msr_is_supported()) return 0;
    
    u64 misc_enable = msr_read(MSR_IA32_MISC_ENABLE);
    return (misc_enable & MISC_ENABLE_TURBO_DISABLE) == 0;
}

void freq_scaling_set_energy_perf_bias(u8 bias) {
    if (!freq_scaling_info.eist_supported || !msr_is_supported()) return;
    if (bias > 15) bias = 15;
    
    msr_write(MSR_IA32_ENERGY_PERF_BIAS, bias);
    freq_scaling_info.energy_perf_bias = bias;
}

u8 freq_scaling_get_energy_perf_bias(void) {
    if (!freq_scaling_info.eist_supported || !msr_is_supported()) return 0;
    
    return msr_read(MSR_IA32_ENERGY_PERF_BIAS) & 0xF;
}

u16 freq_scaling_get_max_frequency(void) {
    return freq_scaling_info.max_frequency_mhz;
}

u16 freq_scaling_get_base_frequency(void) {
    return freq_scaling_info.base_frequency_mhz;
}

u8 freq_scaling_get_num_pstates(void) {
    return freq_scaling_info.num_pstates;
}

const pstate_info_t* freq_scaling_get_pstate_info(u8 pstate) {
    if (pstate >= freq_scaling_info.num_pstates) return 0;
    return &freq_scaling_info.pstates[pstate];
}

u32 freq_scaling_get_transition_count(void) {
    return freq_scaling_info.frequency_transitions;
}

u32 freq_scaling_get_turbo_activations(void) {
    return freq_scaling_info.turbo_activations;
}

u64 freq_scaling_get_last_transition_time(void) {
    return freq_scaling_info.last_transition_time;
}

void freq_scaling_set_performance_mode(void) {
    if (!freq_scaling_info.freq_scaling_supported) return;
    
    freq_scaling_set_frequency(freq_scaling_info.max_frequency_mhz);
    freq_scaling_set_energy_perf_bias(EPB_PERFORMANCE);
    freq_scaling_enable_turbo();
}

void freq_scaling_set_power_save_mode(void) {
    if (!freq_scaling_info.freq_scaling_supported) return;
    
    freq_scaling_set_frequency(freq_scaling_info.base_frequency_mhz);
    freq_scaling_set_energy_perf_bias(EPB_POWER_SAVE);
    freq_scaling_disable_turbo();
}

void freq_scaling_set_balanced_mode(void) {
    if (!freq_scaling_info.freq_scaling_supported) return;
    
    u16 balanced_freq = (freq_scaling_info.max_frequency_mhz + freq_scaling_info.base_frequency_mhz) / 2;
    freq_scaling_set_frequency(balanced_freq);
    freq_scaling_set_energy_perf_bias(EPB_BALANCED_PERFORMANCE);
}

u8 freq_scaling_get_current_pstate(void) {
    return freq_scaling_info.current_pstate;
}

u8 freq_scaling_is_thermal_throttling(void) {
    return freq_scaling_info.thermal_throttling;
}

u8 freq_scaling_is_power_limit_throttling(void) {
    return freq_scaling_info.power_limit_throttling;
}
