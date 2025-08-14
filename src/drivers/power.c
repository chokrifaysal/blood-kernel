/*
 * power.c – x86 CPU power management (P-states, C-states)
 */

#include "kernel/types.h"

/* Power management MSRs */
#define MSR_PERF_STATUS         0x198
#define MSR_PERF_CTL            0x199
#define MSR_CLOCK_MODULATION    0x19A
#define MSR_MISC_ENABLE         0x1A0
#define MSR_ENERGY_PERF_BIAS    0x1B0
#define MSR_PKG_POWER_LIMIT     0x610
#define MSR_PKG_ENERGY_STATUS   0x611
#define MSR_PKG_POWER_INFO      0x614
#define MSR_DRAM_POWER_LIMIT    0x618
#define MSR_DRAM_ENERGY_STATUS  0x619
#define MSR_DRAM_POWER_INFO     0x61C
#define MSR_PP0_POWER_LIMIT     0x638
#define MSR_PP0_ENERGY_STATUS   0x639
#define MSR_PP1_POWER_LIMIT     0x640
#define MSR_PP1_ENERGY_STATUS   0x641

/* ACPI P-state control */
#define ACPI_PSTATE_CONTROL     0x00
#define ACPI_PSTATE_STATUS      0x04

/* Clock modulation bits */
#define CLOCK_MOD_ENABLE        0x10
#define CLOCK_MOD_DUTY_MASK     0x0E

/* Energy performance bias values */
#define EPB_PERFORMANCE         0x00
#define EPB_BALANCED_PERF       0x04
#define EPB_BALANCED            0x06
#define EPB_BALANCED_POWER      0x08
#define EPB_POWER_SAVE          0x0F

typedef struct {
    u16 frequency;
    u16 voltage;
    u16 power;
    u8 valid;
} pstate_t;

typedef struct {
    u8 type;
    u8 latency;
    u16 power;
    u32 address;
} cstate_t;

typedef struct {
    u8 pstate_supported;
    u8 cstate_supported;
    u8 eist_supported;
    u8 turbo_supported;
    u8 clock_modulation_supported;
    u8 energy_perf_bias_supported;
    u8 current_pstate;
    u8 max_pstate;
    u8 min_pstate;
    u8 turbo_pstate;
    pstate_t pstates[16];
    cstate_t cstates[8];
    u8 num_pstates;
    u8 num_cstates;
    u32 energy_unit;
    u32 power_unit;
    u32 time_unit;
} power_info_t;

static power_info_t power_info;

extern u64 msr_read(u32 msr);
extern void msr_write(u32 msr, u64 value);
extern u8 msr_is_supported(void);

static u8 power_check_eist_support(void) {
    u32 eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    
    return (ecx & (1 << 7)) != 0;
}

static u8 power_check_turbo_support(void) {
    u32 eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(6));
    
    return (eax & (1 << 1)) != 0;
}

static u8 power_check_clock_modulation(void) {
    u32 eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    
    return (edx & (1 << 22)) != 0;
}

static u8 power_check_energy_perf_bias(void) {
    u32 eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(6));
    
    return (ecx & (1 << 3)) != 0;
}

static void power_detect_pstates(void) {
    if (!power_info.eist_supported) return;
    
    u64 perf_status = msr_read(MSR_PERF_STATUS);
    power_info.current_pstate = (perf_status >> 8) & 0xFF;
    
    /* Try to detect available P-states by reading platform info */
    u64 platform_info = msr_read(0xCE);
    if (platform_info != 0) {
        power_info.max_pstate = (platform_info >> 8) & 0xFF;
        power_info.min_pstate = (platform_info >> 40) & 0xFF;
        
        if (power_info.turbo_supported) {
            power_info.turbo_pstate = power_info.max_pstate + 1;
        }
    } else {
        power_info.max_pstate = 16;
        power_info.min_pstate = 6;
    }
    
    /* Populate basic P-state table */
    power_info.num_pstates = 0;
    for (u8 i = power_info.min_pstate; i <= power_info.max_pstate && i < 16; i++) {
        power_info.pstates[power_info.num_pstates].frequency = i * 100;
        power_info.pstates[power_info.num_pstates].voltage = 1000 + (i * 50);
        power_info.pstates[power_info.num_pstates].power = i * 5;
        power_info.pstates[power_info.num_pstates].valid = 1;
        power_info.num_pstates++;
    }
}

static void power_detect_cstates(void) {
    /* Basic C-states detection */
    power_info.num_cstates = 0;
    
    /* C0 - Active */
    power_info.cstates[0].type = 0;
    power_info.cstates[0].latency = 0;
    power_info.cstates[0].power = 100;
    power_info.cstates[0].address = 0;
    power_info.num_cstates++;
    
    /* C1 - Halt */
    power_info.cstates[1].type = 1;
    power_info.cstates[1].latency = 1;
    power_info.cstates[1].power = 50;
    power_info.cstates[1].address = 0;
    power_info.num_cstates++;
    
    /* Check for deeper C-states via CPUID */
    u32 eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(5));
    
    if (eax >= 0x20) {
        /* C2 supported */
        power_info.cstates[2].type = 2;
        power_info.cstates[2].latency = 10;
        power_info.cstates[2].power = 25;
        power_info.cstates[2].address = 0x20;
        power_info.num_cstates++;
    }
    
    if (eax >= 0x30) {
        /* C3 supported */
        power_info.cstates[3].type = 3;
        power_info.cstates[3].latency = 50;
        power_info.cstates[3].power = 10;
        power_info.cstates[3].address = 0x30;
        power_info.num_cstates++;
    }
    
    power_info.cstate_supported = (power_info.num_cstates > 1);
}

static void power_init_energy_units(void) {
    u64 power_unit = msr_read(0x606);
    
    power_info.power_unit = 1 << ((power_unit >> 0) & 0xF);
    power_info.energy_unit = 1 << ((power_unit >> 8) & 0x1F);
    power_info.time_unit = 1 << ((power_unit >> 16) & 0xF);
}

void power_init(void) {
    if (!msr_is_supported()) return;
    
    power_info.eist_supported = power_check_eist_support();
    power_info.turbo_supported = power_check_turbo_support();
    power_info.clock_modulation_supported = power_check_clock_modulation();
    power_info.energy_perf_bias_supported = power_check_energy_perf_bias();
    
    power_detect_pstates();
    power_detect_cstates();
    power_init_energy_units();
    
    power_info.pstate_supported = power_info.eist_supported && (power_info.num_pstates > 1);
    
    /* Set balanced energy performance bias */
    if (power_info.energy_perf_bias_supported) {
        msr_write(MSR_ENERGY_PERF_BIAS, EPB_BALANCED);
    }
}

u8 power_set_pstate(u8 pstate) {
    if (!power_info.pstate_supported) return 0;
    
    if (pstate < power_info.min_pstate || pstate > power_info.max_pstate) {
        return 0;
    }
    
    u64 perf_ctl = msr_read(MSR_PERF_CTL);
    perf_ctl = (perf_ctl & ~0xFF00) | ((u64)pstate << 8);
    msr_write(MSR_PERF_CTL, perf_ctl);
    
    power_info.current_pstate = pstate;
    return 1;
}

u8 power_get_pstate(void) {
    if (!power_info.pstate_supported) return 0;
    
    u64 perf_status = msr_read(MSR_PERF_STATUS);
    return (perf_status >> 8) & 0xFF;
}

void power_set_frequency(u16 frequency_mhz) {
    if (!power_info.pstate_supported) return;
    
    u8 target_pstate = frequency_mhz / 100;
    
    if (target_pstate < power_info.min_pstate) {
        target_pstate = power_info.min_pstate;
    } else if (target_pstate > power_info.max_pstate) {
        target_pstate = power_info.max_pstate;
    }
    
    power_set_pstate(target_pstate);
}

u16 power_get_frequency(void) {
    if (!power_info.pstate_supported) return 0;
    
    u8 current_pstate = power_get_pstate();
    return current_pstate * 100;
}

void power_enable_turbo(u8 enable) {
    if (!power_info.turbo_supported) return;
    
    u64 misc_enable = msr_read(MSR_MISC_ENABLE);
    
    if (enable) {
        misc_enable &= ~(1ULL << 38);
    } else {
        misc_enable |= (1ULL << 38);
    }
    
    msr_write(MSR_MISC_ENABLE, misc_enable);
}

u8 power_is_turbo_enabled(void) {
    if (!power_info.turbo_supported) return 0;
    
    u64 misc_enable = msr_read(MSR_MISC_ENABLE);
    return !(misc_enable & (1ULL << 38));
}

void power_set_clock_modulation(u8 duty_cycle) {
    if (!power_info.clock_modulation_supported) return;
    
    if (duty_cycle > 14) duty_cycle = 14;
    
    u64 clock_mod = msr_read(MSR_CLOCK_MODULATION);
    
    if (duty_cycle == 0) {
        clock_mod &= ~CLOCK_MOD_ENABLE;
    } else {
        clock_mod = (clock_mod & ~CLOCK_MOD_DUTY_MASK) | 
                   ((duty_cycle & 0x0E) | CLOCK_MOD_ENABLE);
    }
    
    msr_write(MSR_CLOCK_MODULATION, clock_mod);
}

u8 power_get_clock_modulation(void) {
    if (!power_info.clock_modulation_supported) return 0;
    
    u64 clock_mod = msr_read(MSR_CLOCK_MODULATION);
    
    if (!(clock_mod & CLOCK_MOD_ENABLE)) {
        return 0;
    }
    
    return (clock_mod & CLOCK_MOD_DUTY_MASK);
}

void power_set_energy_perf_bias(u8 bias) {
    if (!power_info.energy_perf_bias_supported) return;
    
    if (bias > 15) bias = 15;
    
    msr_write(MSR_ENERGY_PERF_BIAS, bias);
}

u8 power_get_energy_perf_bias(void) {
    if (!power_info.energy_perf_bias_supported) return 0;
    
    return msr_read(MSR_ENERGY_PERF_BIAS) & 0x0F;
}

void power_enter_cstate(u8 cstate) {
    if (!power_info.cstate_supported || cstate >= power_info.num_cstates) {
        return;
    }
    
    switch (cstate) {
        case 0:
            break;
        case 1:
            __asm__ volatile("hlt");
            break;
        case 2:
        case 3:
            if (power_info.cstates[cstate].address != 0) {
                __asm__ volatile("inb %0, %%al" : : "N"(power_info.cstates[cstate].address) : "al");
            } else {
                __asm__ volatile("hlt");
            }
            break;
        default:
            __asm__ volatile("hlt");
            break;
    }
}

u32 power_get_package_energy(void) {
    u64 energy = msr_read(MSR_PKG_ENERGY_STATUS);
    return (u32)(energy & 0xFFFFFFFF);
}

u32 power_get_dram_energy(void) {
    u64 energy = msr_read(MSR_DRAM_ENERGY_STATUS);
    return (u32)(energy & 0xFFFFFFFF);
}

u8 power_is_pstate_supported(void) {
    return power_info.pstate_supported;
}

u8 power_is_cstate_supported(void) {
    return power_info.cstate_supported;
}

u8 power_get_num_pstates(void) {
    return power_info.num_pstates;
}

u8 power_get_num_cstates(void) {
    return power_info.num_cstates;
}

u8 power_get_max_pstate(void) {
    return power_info.max_pstate;
}

u8 power_get_min_pstate(void) {
    return power_info.min_pstate;
}
