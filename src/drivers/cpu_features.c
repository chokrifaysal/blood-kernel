/*
 * cpu_features.c – x86 CPU feature control and model-specific management
 */

#include "kernel/types.h"

/* Feature control MSRs */
#define MSR_IA32_FEATURE_CONTROL    0x3A
#define MSR_IA32_MISC_ENABLE        0x1A0
#define MSR_IA32_ARCH_CAPABILITIES  0x10A
#define MSR_IA32_CORE_CAPABILITIES  0xCF

/* Intel specific MSRs */
#define MSR_INTEL_PLATFORM_INFO     0xCE
#define MSR_INTEL_TURBO_RATIO_LIMIT 0x1AD
#define MSR_INTEL_ENERGY_PERF_BIAS  0x1B0
#define MSR_INTEL_PACKAGE_THERM_STATUS 0x1B1

/* AMD specific MSRs */
#define MSR_AMD_HWCR                0xC0010015
#define MSR_AMD_PATCH_LEVEL         0x0000008B
#define MSR_AMD_NB_CFG              0xC001001F

/* Feature control bits */
#define FEATURE_CONTROL_LOCKED      0x01
#define FEATURE_CONTROL_VMX_IN_SMX  0x02
#define FEATURE_CONTROL_VMX_OUT_SMX 0x04

/* MISC_ENABLE bits */
#define MISC_ENABLE_FAST_STRING     0x01
#define MISC_ENABLE_TCC             0x08
#define MISC_ENABLE_EMON            0x80
#define MISC_ENABLE_BTS_UNAVAIL     0x800
#define MISC_ENABLE_PEBS_UNAVAIL    0x1000
#define MISC_ENABLE_ENHANCED_SPEEDSTEP 0x10000
#define MISC_ENABLE_MONITOR_FSM     0x40000
#define MISC_ENABLE_LIMIT_CPUID     0x400000
#define MISC_ENABLE_XTPR_DISABLE    0x800000
#define MISC_ENABLE_XD_DISABLE      0x4000000000ULL

typedef struct {
    u8 feature_control_supported;
    u8 misc_enable_supported;
    u8 arch_capabilities_supported;
    u8 core_capabilities_supported;
    u8 intel_features;
    u8 amd_features;
    u64 feature_control_value;
    u64 misc_enable_value;
    u64 arch_capabilities_value;
    u64 core_capabilities_value;
    u8 vmx_locked;
    u8 smx_supported;
    u8 speedstep_enabled;
    u8 turbo_enabled;
    u8 xd_enabled;
    u8 fast_strings_enabled;
    u8 thermal_control_enabled;
    u8 energy_perf_bias_supported;
    u32 max_non_turbo_ratio;
    u32 max_turbo_ratio;
} cpu_features_info_t;

static cpu_features_info_t cpu_features_info;

extern u64 msr_read(u32 msr);
extern void msr_write(u32 msr, u64 value);
extern u8 msr_is_supported(void);

static u8 cpu_features_detect_vendor(void) {
    u32 eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0));
    
    if (ebx == 0x756E6547 && edx == 0x49656E69 && ecx == 0x6C65746E) {
        cpu_features_info.intel_features = 1;
        return 1;
    } else if (ebx == 0x68747541 && edx == 0x69746E65 && ecx == 0x444D4163) {
        cpu_features_info.amd_features = 1;
        return 1;
    }
    
    return 0;
}

static void cpu_features_detect_capabilities(void) {
    u32 eax, ebx, ecx, edx;
    
    if (!msr_is_supported()) return;
    
    /* Check for VMX and SMX */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    
    if (ecx & (1 << 5)) { /* VMX */
        cpu_features_info.feature_control_supported = 1;
    }
    if (ecx & (1 << 6)) { /* SMX */
        cpu_features_info.smx_supported = 1;
    }
    
    /* Check for architectural capabilities */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    cpu_features_info.arch_capabilities_supported = (edx & (1 << 29)) != 0;
    cpu_features_info.core_capabilities_supported = (edx & (1 << 30)) != 0;
    
    /* Check for energy performance bias */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(6));
    cpu_features_info.energy_perf_bias_supported = (ecx & (1 << 3)) != 0;
}

static void cpu_features_read_msr_values(void) {
    if (!msr_is_supported()) return;
    
    /* Read feature control MSR */
    if (cpu_features_info.feature_control_supported) {
        cpu_features_info.feature_control_value = msr_read(MSR_IA32_FEATURE_CONTROL);
        cpu_features_info.vmx_locked = (cpu_features_info.feature_control_value & FEATURE_CONTROL_LOCKED) != 0;
    }
    
    /* Read MISC_ENABLE MSR */
    cpu_features_info.misc_enable_supported = 1;
    cpu_features_info.misc_enable_value = msr_read(MSR_IA32_MISC_ENABLE);
    
    /* Parse MISC_ENABLE bits */
    cpu_features_info.fast_strings_enabled = (cpu_features_info.misc_enable_value & MISC_ENABLE_FAST_STRING) != 0;
    cpu_features_info.thermal_control_enabled = (cpu_features_info.misc_enable_value & MISC_ENABLE_TCC) != 0;
    cpu_features_info.speedstep_enabled = (cpu_features_info.misc_enable_value & MISC_ENABLE_ENHANCED_SPEEDSTEP) != 0;
    cpu_features_info.xd_enabled = !(cpu_features_info.misc_enable_value & MISC_ENABLE_XD_DISABLE);
    
    /* Read architectural capabilities */
    if (cpu_features_info.arch_capabilities_supported) {
        cpu_features_info.arch_capabilities_value = msr_read(MSR_IA32_ARCH_CAPABILITIES);
    }
    
    /* Read core capabilities */
    if (cpu_features_info.core_capabilities_supported) {
        cpu_features_info.core_capabilities_value = msr_read(MSR_IA32_CORE_CAPABILITIES);
    }
    
    /* Intel specific features */
    if (cpu_features_info.intel_features) {
        u64 platform_info = msr_read(MSR_INTEL_PLATFORM_INFO);
        cpu_features_info.max_non_turbo_ratio = (platform_info >> 8) & 0xFF;
        
        u64 turbo_limit = msr_read(MSR_INTEL_TURBO_RATIO_LIMIT);
        cpu_features_info.max_turbo_ratio = turbo_limit & 0xFF;
        
        cpu_features_info.turbo_enabled = !(cpu_features_info.misc_enable_value & (1ULL << 38));
    }
}

void cpu_features_init(void) {
    cpu_features_info.feature_control_supported = 0;
    cpu_features_info.misc_enable_supported = 0;
    cpu_features_info.arch_capabilities_supported = 0;
    cpu_features_info.core_capabilities_supported = 0;
    cpu_features_info.intel_features = 0;
    cpu_features_info.amd_features = 0;
    cpu_features_info.vmx_locked = 0;
    cpu_features_info.smx_supported = 0;
    cpu_features_info.energy_perf_bias_supported = 0;
    
    if (!cpu_features_detect_vendor()) return;
    
    cpu_features_detect_capabilities();
    cpu_features_read_msr_values();
}

u8 cpu_features_is_intel(void) {
    return cpu_features_info.intel_features;
}

u8 cpu_features_is_amd(void) {
    return cpu_features_info.amd_features;
}

u8 cpu_features_is_vmx_supported(void) {
    return cpu_features_info.feature_control_supported;
}

u8 cpu_features_is_vmx_locked(void) {
    return cpu_features_info.vmx_locked;
}

u8 cpu_features_is_smx_supported(void) {
    return cpu_features_info.smx_supported;
}

u8 cpu_features_enable_vmx(void) {
    if (!cpu_features_info.feature_control_supported || !msr_is_supported()) return 0;
    if (cpu_features_info.vmx_locked) return 0;
    
    u64 feature_control = cpu_features_info.feature_control_value;
    feature_control |= FEATURE_CONTROL_VMX_OUT_SMX;
    if (cpu_features_info.smx_supported) {
        feature_control |= FEATURE_CONTROL_VMX_IN_SMX;
    }
    feature_control |= FEATURE_CONTROL_LOCKED;
    
    msr_write(MSR_IA32_FEATURE_CONTROL, feature_control);
    cpu_features_info.feature_control_value = feature_control;
    cpu_features_info.vmx_locked = 1;
    
    return 1;
}

u8 cpu_features_is_speedstep_enabled(void) {
    return cpu_features_info.speedstep_enabled;
}

u8 cpu_features_enable_speedstep(void) {
    if (!cpu_features_info.misc_enable_supported || !msr_is_supported()) return 0;
    
    u64 misc_enable = cpu_features_info.misc_enable_value;
    misc_enable |= MISC_ENABLE_ENHANCED_SPEEDSTEP;
    
    msr_write(MSR_IA32_MISC_ENABLE, misc_enable);
    cpu_features_info.misc_enable_value = misc_enable;
    cpu_features_info.speedstep_enabled = 1;
    
    return 1;
}

u8 cpu_features_disable_speedstep(void) {
    if (!cpu_features_info.misc_enable_supported || !msr_is_supported()) return 0;
    
    u64 misc_enable = cpu_features_info.misc_enable_value;
    misc_enable &= ~MISC_ENABLE_ENHANCED_SPEEDSTEP;
    
    msr_write(MSR_IA32_MISC_ENABLE, misc_enable);
    cpu_features_info.misc_enable_value = misc_enable;
    cpu_features_info.speedstep_enabled = 0;
    
    return 1;
}

u8 cpu_features_is_turbo_enabled(void) {
    return cpu_features_info.turbo_enabled;
}

u8 cpu_features_enable_turbo(void) {
    if (!cpu_features_info.intel_features || !msr_is_supported()) return 0;
    
    u64 misc_enable = cpu_features_info.misc_enable_value;
    misc_enable &= ~(1ULL << 38); /* Clear turbo disable bit */
    
    msr_write(MSR_IA32_MISC_ENABLE, misc_enable);
    cpu_features_info.misc_enable_value = misc_enable;
    cpu_features_info.turbo_enabled = 1;
    
    return 1;
}

u8 cpu_features_disable_turbo(void) {
    if (!cpu_features_info.intel_features || !msr_is_supported()) return 0;
    
    u64 misc_enable = cpu_features_info.misc_enable_value;
    misc_enable |= (1ULL << 38); /* Set turbo disable bit */
    
    msr_write(MSR_IA32_MISC_ENABLE, misc_enable);
    cpu_features_info.misc_enable_value = misc_enable;
    cpu_features_info.turbo_enabled = 0;
    
    return 1;
}

u8 cpu_features_is_xd_enabled(void) {
    return cpu_features_info.xd_enabled;
}

u8 cpu_features_enable_xd(void) {
    if (!cpu_features_info.misc_enable_supported || !msr_is_supported()) return 0;
    
    u64 misc_enable = cpu_features_info.misc_enable_value;
    misc_enable &= ~MISC_ENABLE_XD_DISABLE;
    
    msr_write(MSR_IA32_MISC_ENABLE, misc_enable);
    cpu_features_info.misc_enable_value = misc_enable;
    cpu_features_info.xd_enabled = 1;
    
    return 1;
}

u8 cpu_features_disable_xd(void) {
    if (!cpu_features_info.misc_enable_supported || !msr_is_supported()) return 0;
    
    u64 misc_enable = cpu_features_info.misc_enable_value;
    misc_enable |= MISC_ENABLE_XD_DISABLE;
    
    msr_write(MSR_IA32_MISC_ENABLE, misc_enable);
    cpu_features_info.misc_enable_value = misc_enable;
    cpu_features_info.xd_enabled = 0;
    
    return 1;
}

u8 cpu_features_is_fast_strings_enabled(void) {
    return cpu_features_info.fast_strings_enabled;
}

u8 cpu_features_is_thermal_control_enabled(void) {
    return cpu_features_info.thermal_control_enabled;
}

u32 cpu_features_get_max_non_turbo_ratio(void) {
    return cpu_features_info.max_non_turbo_ratio;
}

u32 cpu_features_get_max_turbo_ratio(void) {
    return cpu_features_info.max_turbo_ratio;
}

u64 cpu_features_get_arch_capabilities(void) {
    return cpu_features_info.arch_capabilities_value;
}

u64 cpu_features_get_core_capabilities(void) {
    return cpu_features_info.core_capabilities_value;
}

u8 cpu_features_is_energy_perf_bias_supported(void) {
    return cpu_features_info.energy_perf_bias_supported;
}

u8 cpu_features_set_energy_perf_bias(u8 bias) {
    if (!cpu_features_info.energy_perf_bias_supported || !msr_is_supported()) return 0;
    if (bias > 15) return 0;
    
    msr_write(MSR_INTEL_ENERGY_PERF_BIAS, bias);
    return 1;
}

u8 cpu_features_get_energy_perf_bias(void) {
    if (!cpu_features_info.energy_perf_bias_supported || !msr_is_supported()) return 0;
    
    return msr_read(MSR_INTEL_ENERGY_PERF_BIAS) & 0xF;
}

u64 cpu_features_get_misc_enable(void) {
    return cpu_features_info.misc_enable_value;
}

u64 cpu_features_get_feature_control(void) {
    return cpu_features_info.feature_control_value;
}

u8 cpu_features_limit_cpuid_maxval(u8 enable) {
    if (!cpu_features_info.misc_enable_supported || !msr_is_supported()) return 0;
    
    u64 misc_enable = cpu_features_info.misc_enable_value;
    
    if (enable) {
        misc_enable |= MISC_ENABLE_LIMIT_CPUID;
    } else {
        misc_enable &= ~MISC_ENABLE_LIMIT_CPUID;
    }
    
    msr_write(MSR_IA32_MISC_ENABLE, misc_enable);
    cpu_features_info.misc_enable_value = misc_enable;
    
    return 1;
}
