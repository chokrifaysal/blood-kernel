/*
 * power_states.c – x86 CPU power state management (C-states/P-states extensions)
 */

#include "kernel/types.h"

/* C-state MSRs */
#define MSR_PKG_CST_CONFIG_CONTROL      0xE2
#define MSR_PMG_IO_CAPTURE_BASE         0xE4
#define MSR_CORE_C3_RESIDENCY           0x3FC
#define MSR_CORE_C6_RESIDENCY           0x3FD
#define MSR_CORE_C7_RESIDENCY           0x3FE
#define MSR_PKG_C2_RESIDENCY            0x60D
#define MSR_PKG_C3_RESIDENCY            0x3F8
#define MSR_PKG_C6_RESIDENCY            0x3F9
#define MSR_PKG_C7_RESIDENCY            0x3FA

/* P-state MSRs */
#define MSR_PERF_CTL                    0x199
#define MSR_PERF_STATUS                 0x198
#define MSR_THERM_STATUS                0x19C
#define MSR_THERM_INTERRUPT             0x19B

/* Power control bits */
#define PKG_CST_CONFIG_LIMIT_MASK       0x7
#define PKG_CST_CONFIG_IO_MWAIT         (1 << 4)
#define PKG_CST_CONFIG_CFG_LOCK         (1 << 15)

/* C-state definitions */
#define CSTATE_C0                       0
#define CSTATE_C1                       1
#define CSTATE_C1E                      2
#define CSTATE_C3                       3
#define CSTATE_C6                       6
#define CSTATE_C7                       7
#define CSTATE_C8                       8
#define CSTATE_C9                       9
#define CSTATE_C10                      10

/* P-state definitions */
#define PSTATE_P0                       0  /* Maximum performance */
#define PSTATE_P1                       1
#define PSTATE_P2                       2
#define PSTATE_P3                       3
#define PSTATE_P4                       4
#define PSTATE_P5                       5
#define PSTATE_P6                       6
#define PSTATE_P7                       7  /* Minimum performance */

typedef struct {
    u8 state;
    u64 entry_time;
    u64 exit_time;
    u64 residency_time;
    u32 entry_count;
    u32 exit_latency_us;
    u8 available;
} cstate_info_t;

typedef struct {
    u8 state;
    u16 frequency_mhz;
    u16 voltage_mv;
    u32 power_mw;
    u64 transition_time;
    u32 transition_count;
    u32 transition_latency_us;
    u8 available;
} pstate_info_t;

typedef struct {
    u8 power_states_supported;
    u8 cstates_supported;
    u8 pstates_supported;
    u8 current_cstate;
    u8 current_pstate;
    u8 max_cstate;
    u8 max_pstate;
    u8 num_cstates;
    u8 num_pstates;
    cstate_info_t cstates[16];
    pstate_info_t pstates[16];
    u64 total_power_transitions;
    u64 total_cstate_residency;
    u64 total_pstate_time;
    u32 thermal_throttle_count;
    u32 power_limit_throttle_count;
    u64 last_transition_time;
    u16 current_temperature;
    u16 max_temperature;
    u8 power_management_enabled;
} power_states_info_t;

static power_states_info_t power_states_info;

extern u64 msr_read(u32 msr);
extern void msr_write(u32 msr, u64 value);
extern u8 msr_is_supported(void);
extern u64 timer_get_ticks(void);

static void power_states_detect_cstates(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check for MWAIT support */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    power_states_info.cstates_supported = (ecx & (1 << 3)) != 0;
    
    if (!power_states_info.cstates_supported) return;
    
    /* Enumerate MWAIT C-states */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(5));
    
    u8 cstate_count = 0;
    
    /* C0 is always available */
    power_states_info.cstates[cstate_count].state = CSTATE_C0;
    power_states_info.cstates[cstate_count].available = 1;
    power_states_info.cstates[cstate_count].exit_latency_us = 0;
    cstate_count++;
    
    /* C1 is typically available */
    power_states_info.cstates[cstate_count].state = CSTATE_C1;
    power_states_info.cstates[cstate_count].available = 1;
    power_states_info.cstates[cstate_count].exit_latency_us = 1;
    cstate_count++;
    
    /* Check for deeper C-states based on CPUID */
    if (ecx & 0xF0) {  /* C3 sub-states */
        power_states_info.cstates[cstate_count].state = CSTATE_C3;
        power_states_info.cstates[cstate_count].available = 1;
        power_states_info.cstates[cstate_count].exit_latency_us = 50;
        cstate_count++;
    }
    
    if (ecx & 0xF00) {  /* C6 sub-states */
        power_states_info.cstates[cstate_count].state = CSTATE_C6;
        power_states_info.cstates[cstate_count].available = 1;
        power_states_info.cstates[cstate_count].exit_latency_us = 100;
        cstate_count++;
    }
    
    if (ecx & 0xF000) {  /* C7 sub-states */
        power_states_info.cstates[cstate_count].state = CSTATE_C7;
        power_states_info.cstates[cstate_count].available = 1;
        power_states_info.cstates[cstate_count].exit_latency_us = 200;
        cstate_count++;
    }
    
    power_states_info.num_cstates = cstate_count;
    power_states_info.max_cstate = power_states_info.cstates[cstate_count - 1].state;
}

static void power_states_detect_pstates(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check for SpeedStep/PowerNow support */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    power_states_info.pstates_supported = (ecx & (1 << 7)) != 0;  /* EIST */
    
    if (!power_states_info.pstates_supported) return;
    
    /* Enumerate P-states (simplified) */
    u8 pstate_count = 0;
    
    for (u8 i = 0; i < 8; i++) {
        power_states_info.pstates[pstate_count].state = i;
        power_states_info.pstates[pstate_count].frequency_mhz = 3000 - (i * 300);  /* Example frequencies */
        power_states_info.pstates[pstate_count].voltage_mv = 1200 - (i * 100);     /* Example voltages */
        power_states_info.pstates[pstate_count].power_mw = 65000 - (i * 8000);     /* Example power */
        power_states_info.pstates[pstate_count].transition_latency_us = 10;
        power_states_info.pstates[pstate_count].available = 1;
        pstate_count++;
    }
    
    power_states_info.num_pstates = pstate_count;
    power_states_info.max_pstate = pstate_count - 1;
}

void power_states_init(void) {
    power_states_info.power_states_supported = 0;
    power_states_info.cstates_supported = 0;
    power_states_info.pstates_supported = 0;
    power_states_info.current_cstate = CSTATE_C0;
    power_states_info.current_pstate = PSTATE_P0;
    power_states_info.max_cstate = CSTATE_C0;
    power_states_info.max_pstate = PSTATE_P0;
    power_states_info.num_cstates = 0;
    power_states_info.num_pstates = 0;
    power_states_info.total_power_transitions = 0;
    power_states_info.total_cstate_residency = 0;
    power_states_info.total_pstate_time = 0;
    power_states_info.thermal_throttle_count = 0;
    power_states_info.power_limit_throttle_count = 0;
    power_states_info.last_transition_time = 0;
    power_states_info.current_temperature = 0;
    power_states_info.max_temperature = 0;
    power_states_info.power_management_enabled = 1;
    
    for (u8 i = 0; i < 16; i++) {
        power_states_info.cstates[i].state = 0;
        power_states_info.cstates[i].entry_time = 0;
        power_states_info.cstates[i].exit_time = 0;
        power_states_info.cstates[i].residency_time = 0;
        power_states_info.cstates[i].entry_count = 0;
        power_states_info.cstates[i].exit_latency_us = 0;
        power_states_info.cstates[i].available = 0;
        
        power_states_info.pstates[i].state = 0;
        power_states_info.pstates[i].frequency_mhz = 0;
        power_states_info.pstates[i].voltage_mv = 0;
        power_states_info.pstates[i].power_mw = 0;
        power_states_info.pstates[i].transition_time = 0;
        power_states_info.pstates[i].transition_count = 0;
        power_states_info.pstates[i].transition_latency_us = 0;
        power_states_info.pstates[i].available = 0;
    }
    
    power_states_detect_cstates();
    power_states_detect_pstates();
    
    if (power_states_info.cstates_supported || power_states_info.pstates_supported) {
        power_states_info.power_states_supported = 1;
    }
}

u8 power_states_is_supported(void) {
    return power_states_info.power_states_supported;
}

u8 power_states_is_cstates_supported(void) {
    return power_states_info.cstates_supported;
}

u8 power_states_is_pstates_supported(void) {
    return power_states_info.pstates_supported;
}

u8 power_states_enter_cstate(u8 cstate) {
    if (!power_states_info.cstates_supported || cstate >= power_states_info.num_cstates) return 0;
    if (!power_states_info.cstates[cstate].available) return 0;
    
    u64 entry_time = timer_get_ticks();
    
    switch (cstate) {
        case CSTATE_C1:
            __asm__ volatile("hlt");
            break;
        case CSTATE_C3:
        case CSTATE_C6:
        case CSTATE_C7:
            /* Use MWAIT for deeper C-states */
            __asm__ volatile("monitor" : : "a"(&power_states_info.current_cstate), "c"(0), "d"(0));
            __asm__ volatile("mwait" : : "a"(cstate << 4), "c"(0));
            break;
        default:
            return 0;
    }
    
    u64 exit_time = timer_get_ticks();
    
    power_states_info.cstates[cstate].entry_time = entry_time;
    power_states_info.cstates[cstate].exit_time = exit_time;
    power_states_info.cstates[cstate].residency_time += (exit_time - entry_time);
    power_states_info.cstates[cstate].entry_count++;
    power_states_info.total_cstate_residency += (exit_time - entry_time);
    power_states_info.total_power_transitions++;
    power_states_info.last_transition_time = exit_time;
    
    return 1;
}

u8 power_states_set_pstate(u8 pstate) {
    if (!power_states_info.pstates_supported || pstate >= power_states_info.num_pstates) return 0;
    if (!power_states_info.pstates[pstate].available || !msr_is_supported()) return 0;
    
    u64 transition_start = timer_get_ticks();
    
    /* Set P-state via PERF_CTL MSR */
    u64 perf_ctl = (u64)pstate << 8;
    msr_write(MSR_PERF_CTL, perf_ctl);
    
    u64 transition_end = timer_get_ticks();
    
    power_states_info.current_pstate = pstate;
    power_states_info.pstates[pstate].transition_time = transition_end;
    power_states_info.pstates[pstate].transition_count++;
    power_states_info.total_power_transitions++;
    power_states_info.last_transition_time = transition_end;
    
    return 1;
}

u8 power_states_get_current_cstate(void) {
    return power_states_info.current_cstate;
}

u8 power_states_get_current_pstate(void) {
    return power_states_info.current_pstate;
}

u8 power_states_get_max_cstate(void) {
    return power_states_info.max_cstate;
}

u8 power_states_get_max_pstate(void) {
    return power_states_info.max_pstate;
}

u8 power_states_get_num_cstates(void) {
    return power_states_info.num_cstates;
}

u8 power_states_get_num_pstates(void) {
    return power_states_info.num_pstates;
}

const cstate_info_t* power_states_get_cstate_info(u8 cstate) {
    if (cstate >= power_states_info.num_cstates) return 0;
    return &power_states_info.cstates[cstate];
}

const pstate_info_t* power_states_get_pstate_info(u8 pstate) {
    if (pstate >= power_states_info.num_pstates) return 0;
    return &power_states_info.pstates[pstate];
}

u64 power_states_get_cstate_residency(u8 cstate) {
    if (!power_states_info.cstates_supported || !msr_is_supported()) return 0;
    
    switch (cstate) {
        case CSTATE_C3:
            return msr_read(MSR_CORE_C3_RESIDENCY);
        case CSTATE_C6:
            return msr_read(MSR_CORE_C6_RESIDENCY);
        case CSTATE_C7:
            return msr_read(MSR_CORE_C7_RESIDENCY);
        default:
            return 0;
    }
}

u16 power_states_get_current_temperature(void) {
    if (!msr_is_supported()) return 0;
    
    u64 therm_status = msr_read(MSR_THERM_STATUS);
    u16 temperature = (therm_status >> 16) & 0x7F;
    
    power_states_info.current_temperature = temperature;
    if (temperature > power_states_info.max_temperature) {
        power_states_info.max_temperature = temperature;
    }
    
    return temperature;
}

u8 power_states_is_thermal_throttling(void) {
    if (!msr_is_supported()) return 0;
    
    u64 therm_status = msr_read(MSR_THERM_STATUS);
    if (therm_status & (1 << 0)) {  /* Thermal status */
        power_states_info.thermal_throttle_count++;
        return 1;
    }
    
    return 0;
}

u64 power_states_get_total_transitions(void) {
    return power_states_info.total_power_transitions;
}

u64 power_states_get_total_cstate_residency(void) {
    return power_states_info.total_cstate_residency;
}

u32 power_states_get_thermal_throttle_count(void) {
    return power_states_info.thermal_throttle_count;
}

u32 power_states_get_power_limit_throttle_count(void) {
    return power_states_info.power_limit_throttle_count;
}

u64 power_states_get_last_transition_time(void) {
    return power_states_info.last_transition_time;
}

u16 power_states_get_max_temperature(void) {
    return power_states_info.max_temperature;
}

void power_states_enable_power_management(void) {
    power_states_info.power_management_enabled = 1;
}

void power_states_disable_power_management(void) {
    power_states_info.power_management_enabled = 0;
}

u8 power_states_is_power_management_enabled(void) {
    return power_states_info.power_management_enabled;
}

void power_states_clear_statistics(void) {
    power_states_info.total_power_transitions = 0;
    power_states_info.total_cstate_residency = 0;
    power_states_info.total_pstate_time = 0;
    power_states_info.thermal_throttle_count = 0;
    power_states_info.power_limit_throttle_count = 0;
    power_states_info.max_temperature = 0;
    
    for (u8 i = 0; i < power_states_info.num_cstates; i++) {
        power_states_info.cstates[i].residency_time = 0;
        power_states_info.cstates[i].entry_count = 0;
    }
    
    for (u8 i = 0; i < power_states_info.num_pstates; i++) {
        power_states_info.pstates[i].transition_count = 0;
    }
}

const char* power_states_get_cstate_name(u8 cstate) {
    switch (cstate) {
        case CSTATE_C0: return "C0 (Active)";
        case CSTATE_C1: return "C1 (Halt)";
        case CSTATE_C1E: return "C1E (Enhanced Halt)";
        case CSTATE_C3: return "C3 (Sleep)";
        case CSTATE_C6: return "C6 (Deep Sleep)";
        case CSTATE_C7: return "C7 (Deeper Sleep)";
        case CSTATE_C8: return "C8 (Deepest Sleep)";
        case CSTATE_C9: return "C9 (Ultra Deep Sleep)";
        case CSTATE_C10: return "C10 (Maximum Power Saving)";
        default: return "Unknown";
    }
}

const char* power_states_get_pstate_name(u8 pstate) {
    switch (pstate) {
        case PSTATE_P0: return "P0 (Maximum Performance)";
        case PSTATE_P1: return "P1 (High Performance)";
        case PSTATE_P2: return "P2 (Medium Performance)";
        case PSTATE_P3: return "P3 (Low Performance)";
        case PSTATE_P4: return "P4 (Lower Performance)";
        case PSTATE_P5: return "P5 (Lowest Performance)";
        case PSTATE_P6: return "P6 (Power Saving)";
        case PSTATE_P7: return "P7 (Maximum Power Saving)";
        default: return "Unknown";
    }
}
