/*
 * power_mgmt.c – x86 advanced power management (C-states/P-states deep control)
 */

#include "kernel/types.h"

/* ACPI C-state types */
#define CSTATE_C0           0   /* Active */
#define CSTATE_C1           1   /* Halt */
#define CSTATE_C1E          2   /* Enhanced Halt */
#define CSTATE_C2           3   /* Stop Clock */
#define CSTATE_C3           4   /* Sleep */
#define CSTATE_C4           5   /* Deeper Sleep */
#define CSTATE_C6           6   /* Deep Power Down */
#define CSTATE_C7           7   /* Deeper Power Down */
#define CSTATE_C8           8   /* Deepest Power Down */

/* Power management MSRs */
#define MSR_PKG_CST_CONFIG_CONTROL  0xE2
#define MSR_PMG_IO_CAPTURE_BASE     0xE4
#define MSR_CORE_C3_RESIDENCY       0x3FC
#define MSR_CORE_C6_RESIDENCY       0x3FD
#define MSR_CORE_C7_RESIDENCY       0x3FE
#define MSR_PKG_C2_RESIDENCY        0x60D
#define MSR_PKG_C3_RESIDENCY        0x3F8
#define MSR_PKG_C6_RESIDENCY        0x3F9
#define MSR_PKG_C7_RESIDENCY        0x3FA
#define MSR_PKG_C8_RESIDENCY        0x630
#define MSR_PKG_C9_RESIDENCY        0x631
#define MSR_PKG_C10_RESIDENCY       0x632

/* Power limit MSRs */
#define MSR_PKG_POWER_LIMIT         0x610
#define MSR_PKG_ENERGY_STATUS       0x611
#define MSR_PKG_POWER_INFO          0x614
#define MSR_DRAM_POWER_LIMIT        0x618
#define MSR_DRAM_ENERGY_STATUS      0x619
#define MSR_DRAM_POWER_INFO         0x61C

/* MWAIT hints */
#define MWAIT_C1                    0x00
#define MWAIT_C1E                   0x01
#define MWAIT_C2                    0x10
#define MWAIT_C3                    0x20
#define MWAIT_C4                    0x30
#define MWAIT_C6                    0x50
#define MWAIT_C7                    0x60
#define MWAIT_C8                    0x70

typedef struct {
    u8 type;
    u8 latency;
    u16 power;
    u32 mwait_hint;
    u64 residency_counter;
    u32 usage_count;
} cstate_info_t;

typedef struct {
    u8 power_mgmt_supported;
    u8 cstate_supported;
    u8 enhanced_cstate_supported;
    u8 package_cstate_supported;
    u8 energy_monitoring_supported;
    u8 power_limit_supported;
    u8 num_cstates;
    u8 deepest_cstate;
    u8 current_cstate;
    cstate_info_t cstates[16];
    u64 pkg_energy_unit;
    u64 time_unit;
    u32 power_unit;
    u64 last_energy_reading;
    u64 total_energy_consumed;
    u32 thermal_design_power;
    u32 minimum_power;
    u32 maximum_power;
} power_mgmt_info_t;

static power_mgmt_info_t power_mgmt_info;

extern u64 msr_read(u32 msr);
extern void msr_write(u32 msr, u64 value);
extern u8 msr_is_supported(void);
extern void outb(u16 port, u8 value);
extern u8 inb(u16 port);

static void power_mgmt_detect_capabilities(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check for MWAIT support */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    power_mgmt_info.cstate_supported = (ecx & (1 << 3)) != 0;
    
    if (power_mgmt_info.cstate_supported) {
        /* Get MWAIT capabilities */
        __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(5));
        
        power_mgmt_info.enhanced_cstate_supported = (ecx & (1 << 0)) != 0;
        power_mgmt_info.num_cstates = (edx >> 4) & 0xF;
        
        if (power_mgmt_info.num_cstates > 16) {
            power_mgmt_info.num_cstates = 16;
        }
    }
    
    /* Check for energy monitoring */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(6));
    power_mgmt_info.energy_monitoring_supported = (ecx & (1 << 0)) != 0;
    power_mgmt_info.power_limit_supported = (ecx & (1 << 4)) != 0;
    
    /* Check for package C-states */
    if (msr_is_supported()) {
        u64 pkg_cst_config = msr_read(MSR_PKG_CST_CONFIG_CONTROL);
        power_mgmt_info.package_cstate_supported = (pkg_cst_config & (1ULL << 15)) != 0;
    }
}

static void power_mgmt_setup_energy_units(void) {
    if (!power_mgmt_info.energy_monitoring_supported || !msr_is_supported()) return;
    
    /* Read energy unit from RAPL */
    u64 power_unit = msr_read(MSR_PKG_POWER_INFO);
    
    power_mgmt_info.power_unit = 1 << (power_unit & 0xF);
    power_mgmt_info.pkg_energy_unit = 1 << ((power_unit >> 8) & 0x1F);
    power_mgmt_info.time_unit = 1 << ((power_unit >> 16) & 0xF);
    
    /* Read power info */
    u64 power_info = msr_read(MSR_PKG_POWER_INFO);
    power_mgmt_info.thermal_design_power = (power_info & 0x7FFF) * power_mgmt_info.power_unit;
    power_mgmt_info.minimum_power = ((power_info >> 16) & 0x7FFF) * power_mgmt_info.power_unit;
    power_mgmt_info.maximum_power = ((power_info >> 32) & 0x7FFF) * power_mgmt_info.power_unit;
}

static void power_mgmt_enumerate_cstates(void) {
    if (!power_mgmt_info.cstate_supported) return;
    
    /* Setup basic C-states */
    power_mgmt_info.cstates[0].type = CSTATE_C1;
    power_mgmt_info.cstates[0].latency = 1;
    power_mgmt_info.cstates[0].power = 1000;
    power_mgmt_info.cstates[0].mwait_hint = MWAIT_C1;
    
    if (power_mgmt_info.enhanced_cstate_supported) {
        power_mgmt_info.cstates[1].type = CSTATE_C1E;
        power_mgmt_info.cstates[1].latency = 10;
        power_mgmt_info.cstates[1].power = 500;
        power_mgmt_info.cstates[1].mwait_hint = MWAIT_C1E;
        
        power_mgmt_info.cstates[2].type = CSTATE_C3;
        power_mgmt_info.cstates[2].latency = 50;
        power_mgmt_info.cstates[2].power = 200;
        power_mgmt_info.cstates[2].mwait_hint = MWAIT_C3;
        
        power_mgmt_info.cstates[3].type = CSTATE_C6;
        power_mgmt_info.cstates[3].latency = 100;
        power_mgmt_info.cstates[3].power = 50;
        power_mgmt_info.cstates[3].mwait_hint = MWAIT_C6;
        
        power_mgmt_info.cstates[4].type = CSTATE_C7;
        power_mgmt_info.cstates[4].latency = 200;
        power_mgmt_info.cstates[4].power = 25;
        power_mgmt_info.cstates[4].mwait_hint = MWAIT_C7;
        
        power_mgmt_info.deepest_cstate = CSTATE_C7;
    } else {
        power_mgmt_info.deepest_cstate = CSTATE_C1;
    }
}

void power_mgmt_init(void) {
    power_mgmt_info.power_mgmt_supported = 0;
    power_mgmt_info.cstate_supported = 0;
    power_mgmt_info.enhanced_cstate_supported = 0;
    power_mgmt_info.package_cstate_supported = 0;
    power_mgmt_info.energy_monitoring_supported = 0;
    power_mgmt_info.power_limit_supported = 0;
    power_mgmt_info.num_cstates = 0;
    power_mgmt_info.deepest_cstate = CSTATE_C0;
    power_mgmt_info.current_cstate = CSTATE_C0;
    power_mgmt_info.last_energy_reading = 0;
    power_mgmt_info.total_energy_consumed = 0;
    
    power_mgmt_detect_capabilities();
    power_mgmt_setup_energy_units();
    power_mgmt_enumerate_cstates();
    
    if (power_mgmt_info.cstate_supported || power_mgmt_info.energy_monitoring_supported) {
        power_mgmt_info.power_mgmt_supported = 1;
    }
}

u8 power_mgmt_is_supported(void) {
    return power_mgmt_info.power_mgmt_supported;
}

u8 power_mgmt_is_cstate_supported(void) {
    return power_mgmt_info.cstate_supported;
}

u8 power_mgmt_is_energy_monitoring_supported(void) {
    return power_mgmt_info.energy_monitoring_supported;
}

u8 power_mgmt_get_num_cstates(void) {
    return power_mgmt_info.num_cstates;
}

u8 power_mgmt_get_deepest_cstate(void) {
    return power_mgmt_info.deepest_cstate;
}

u8 power_mgmt_get_current_cstate(void) {
    return power_mgmt_info.current_cstate;
}

const cstate_info_t* power_mgmt_get_cstate_info(u8 cstate) {
    if (cstate >= power_mgmt_info.num_cstates) return 0;
    return &power_mgmt_info.cstates[cstate];
}

void power_mgmt_enter_cstate(u8 cstate) {
    if (!power_mgmt_info.cstate_supported || cstate >= power_mgmt_info.num_cstates) return;
    
    cstate_info_t* state = &power_mgmt_info.cstates[cstate];
    power_mgmt_info.current_cstate = cstate;
    state->usage_count++;
    
    if (cstate == CSTATE_C1) {
        __asm__ volatile("hlt");
    } else if (power_mgmt_info.enhanced_cstate_supported) {
        extern void atomic_monitor(const volatile void* addr);
        extern void atomic_mwait(u32 hints, u32 extensions);
        
        atomic_monitor(&power_mgmt_info.current_cstate);
        atomic_mwait(state->mwait_hint, 0);
    } else {
        __asm__ volatile("hlt");
    }
    
    power_mgmt_info.current_cstate = CSTATE_C0;
}

void power_mgmt_idle(void) {
    /* Choose appropriate C-state based on system load */
    u8 target_cstate = CSTATE_C1;
    
    if (power_mgmt_info.enhanced_cstate_supported) {
        /* Simple policy: use deeper states for longer idle periods */
        target_cstate = CSTATE_C3;
    }
    
    power_mgmt_enter_cstate(target_cstate);
}

u64 power_mgmt_get_cstate_residency(u8 cstate) {
    if (!power_mgmt_info.cstate_supported || !msr_is_supported()) return 0;
    
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

u64 power_mgmt_get_package_cstate_residency(u8 cstate) {
    if (!power_mgmt_info.package_cstate_supported || !msr_is_supported()) return 0;
    
    switch (cstate) {
        case CSTATE_C2:
            return msr_read(MSR_PKG_C2_RESIDENCY);
        case CSTATE_C3:
            return msr_read(MSR_PKG_C3_RESIDENCY);
        case CSTATE_C6:
            return msr_read(MSR_PKG_C6_RESIDENCY);
        case CSTATE_C7:
            return msr_read(MSR_PKG_C7_RESIDENCY);
        case CSTATE_C8:
            return msr_read(MSR_PKG_C8_RESIDENCY);
        default:
            return 0;
    }
}

u32 power_mgmt_get_cstate_usage_count(u8 cstate) {
    if (cstate >= power_mgmt_info.num_cstates) return 0;
    return power_mgmt_info.cstates[cstate].usage_count;
}

u64 power_mgmt_get_energy_consumed(void) {
    if (!power_mgmt_info.energy_monitoring_supported || !msr_is_supported()) return 0;
    
    u64 current_energy = msr_read(MSR_PKG_ENERGY_STATUS);
    
    if (power_mgmt_info.last_energy_reading != 0) {
        u64 delta = current_energy - power_mgmt_info.last_energy_reading;
        power_mgmt_info.total_energy_consumed += delta;
    }
    
    power_mgmt_info.last_energy_reading = current_energy;
    return power_mgmt_info.total_energy_consumed * power_mgmt_info.pkg_energy_unit;
}

u32 power_mgmt_get_current_power(void) {
    if (!power_mgmt_info.energy_monitoring_supported || !msr_is_supported()) return 0;
    
    static u64 last_energy = 0;
    static u64 last_time = 0;
    
    u64 current_energy = msr_read(MSR_PKG_ENERGY_STATUS);
    extern u64 timer_get_ticks(void);
    u64 current_time = timer_get_ticks();
    
    if (last_energy != 0 && last_time != 0) {
        u64 energy_delta = (current_energy - last_energy) * power_mgmt_info.pkg_energy_unit;
        u64 time_delta = current_time - last_time;
        
        if (time_delta > 0) {
            return (u32)(energy_delta / time_delta); /* Power in watts */
        }
    }
    
    last_energy = current_energy;
    last_time = current_time;
    return 0;
}

u32 power_mgmt_get_thermal_design_power(void) {
    return power_mgmt_info.thermal_design_power;
}

u32 power_mgmt_get_minimum_power(void) {
    return power_mgmt_info.minimum_power;
}

u32 power_mgmt_get_maximum_power(void) {
    return power_mgmt_info.maximum_power;
}

void power_mgmt_set_power_limit(u32 power_limit_watts) {
    if (!power_mgmt_info.power_limit_supported || !msr_is_supported()) return;
    
    u64 power_limit = msr_read(MSR_PKG_POWER_LIMIT);
    
    /* Set PL1 (Package Power Limit 1) */
    power_limit &= ~0x7FFF; /* Clear existing limit */
    power_limit |= (power_limit_watts / power_mgmt_info.power_unit) & 0x7FFF;
    power_limit |= (1ULL << 15); /* Enable PL1 */
    
    msr_write(MSR_PKG_POWER_LIMIT, power_limit);
}

u32 power_mgmt_get_power_limit(void) {
    if (!power_mgmt_info.power_limit_supported || !msr_is_supported()) return 0;
    
    u64 power_limit = msr_read(MSR_PKG_POWER_LIMIT);
    return (power_limit & 0x7FFF) * power_mgmt_info.power_unit;
}

void power_mgmt_enable_package_cstates(void) {
    if (!power_mgmt_info.package_cstate_supported || !msr_is_supported()) return;
    
    u64 pkg_cst_config = msr_read(MSR_PKG_CST_CONFIG_CONTROL);
    pkg_cst_config |= (1ULL << 15); /* Enable package C-states */
    pkg_cst_config |= 0x7; /* Allow up to C7 */
    msr_write(MSR_PKG_CST_CONFIG_CONTROL, pkg_cst_config);
}

void power_mgmt_disable_package_cstates(void) {
    if (!power_mgmt_info.package_cstate_supported || !msr_is_supported()) return;
    
    u64 pkg_cst_config = msr_read(MSR_PKG_CST_CONFIG_CONTROL);
    pkg_cst_config &= ~(1ULL << 15); /* Disable package C-states */
    msr_write(MSR_PKG_CST_CONFIG_CONTROL, pkg_cst_config);
}

u8 power_mgmt_are_package_cstates_enabled(void) {
    if (!power_mgmt_info.package_cstate_supported || !msr_is_supported()) return 0;
    
    u64 pkg_cst_config = msr_read(MSR_PKG_CST_CONFIG_CONTROL);
    return (pkg_cst_config & (1ULL << 15)) != 0;
}
