/*
 * msr.c – x86 Model Specific Registers
 */

#include "kernel/types.h"

/* Common MSR addresses */
#define MSR_APIC_BASE           0x1B
#define MSR_FEATURE_CONTROL     0x3A
#define MSR_TSC                 0x10
#define MSR_PLATFORM_ID         0x17
#define MSR_APIC_BASE           0x1B
#define MSR_EBL_CR_POWERON      0x2A
#define MSR_TEST_CTL            0x33
#define MSR_BIOS_UPDT_TRIG      0x79
#define MSR_BIOS_SIGN_ID        0x8B
#define MSR_SMM_MONITOR_CTL     0x9B
#define MSR_PMC0                0xC1
#define MSR_PMC1                0xC2
#define MSR_PMC2                0xC3
#define MSR_PMC3                0xC4
#define MSR_PERFEVTSEL0         0x186
#define MSR_PERFEVTSEL1         0x187
#define MSR_PERFEVTSEL2         0x188
#define MSR_PERFEVTSEL3         0x189
#define MSR_PERF_STATUS         0x198
#define MSR_PERF_CTL            0x199
#define MSR_CLOCK_MODULATION    0x19A
#define MSR_THERM_INTERRUPT     0x19B
#define MSR_THERM_STATUS        0x19C
#define MSR_MISC_ENABLE         0x1A0
#define MSR_ENERGY_PERF_BIAS    0x1B0
#define MSR_PACKAGE_THERM_STATUS 0x1B1
#define MSR_PACKAGE_THERM_INTERRUPT 0x1B2
#define MSR_DEBUGCTL            0x1D9
#define MSR_LASTBRANCHFROMIP    0x1DB
#define MSR_LASTBRANCHTOIP      0x1DC
#define MSR_LASTINTFROMIP       0x1DD
#define MSR_LASTINTTOIP         0x1DE
#define MSR_ROB_CR_BKUPTMPDR6   0x1E0
#define MSR_MTRRPHYSBASE0       0x200
#define MSR_MTRRPHYSMASK0       0x201
#define MSR_MTRRPHYSBASE1       0x202
#define MSR_MTRRPHYSMASK1       0x203
#define MSR_MTRRPHYSBASE2       0x204
#define MSR_MTRRPHYSMASK2       0x205
#define MSR_MTRRPHYSBASE3       0x206
#define MSR_MTRRPHYSMASK3       0x207
#define MSR_MTRRPHYSBASE4       0x208
#define MSR_MTRRPHYSMASK4       0x209
#define MSR_MTRRPHYSBASE5       0x20A
#define MSR_MTRRPHYSMASK5       0x20B
#define MSR_MTRRPHYSBASE6       0x20C
#define MSR_MTRRPHYSMASK6       0x20D
#define MSR_MTRRPHYSBASE7       0x20E
#define MSR_MTRRPHYSMASK7       0x20F
#define MSR_MTRRFIX64K_00000    0x250
#define MSR_MTRRFIX16K_80000    0x258
#define MSR_MTRRFIX16K_A0000    0x259
#define MSR_MTRRFIX4K_C0000     0x268
#define MSR_MTRRFIX4K_C8000     0x269
#define MSR_MTRRFIX4K_D0000     0x26A
#define MSR_MTRRFIX4K_D8000     0x26B
#define MSR_MTRRFIX4K_E0000     0x26C
#define MSR_MTRRFIX4K_E8000     0x26D
#define MSR_MTRRFIX4K_F0000     0x26E
#define MSR_MTRRFIX4K_F8000     0x26F
#define MSR_PAT                 0x277
#define MSR_MTRRDEFTYPE         0x2FF
#define MSR_FIXED_CTR0          0x309
#define MSR_FIXED_CTR1          0x30A
#define MSR_FIXED_CTR2          0x30B
#define MSR_PERF_CAPABILITIES   0x345
#define MSR_FIXED_CTR_CTRL      0x38D
#define MSR_PERF_GLOBAL_STATUS  0x38E
#define MSR_PERF_GLOBAL_CTRL    0x38F
#define MSR_PERF_GLOBAL_OVF_CTRL 0x390
#define MSR_EFER                0xC0000080
#define MSR_STAR                0xC0000081
#define MSR_LSTAR               0xC0000082
#define MSR_CSTAR               0xC0000083
#define MSR_SYSCALL_MASK        0xC0000084
#define MSR_FS_BASE             0xC0000100
#define MSR_GS_BASE             0xC0000101
#define MSR_KERNEL_GS_BASE      0xC0000102

static u8 msr_supported = 0;

static inline u8 msr_check_support(void) {
    u32 eax, edx;
    
    /* Check CPUID for MSR support */
    __asm__ volatile("cpuid" : "=a"(eax), "=d"(edx) : "a"(1) : "ebx", "ecx");
    
    return (edx & (1 << 5)) != 0; /* MSR support bit */
}

void msr_init(void) {
    msr_supported = msr_check_support();
}

u64 msr_read(u32 msr) {
    if (!msr_supported) return 0;
    
    u32 low, high;
    __asm__ volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    
    return ((u64)high << 32) | low;
}

void msr_write(u32 msr, u64 value) {
    if (!msr_supported) return;
    
    u32 low = value & 0xFFFFFFFF;
    u32 high = value >> 32;
    
    __asm__ volatile("wrmsr" : : "a"(low), "d"(high), "c"(msr));
}

u8 msr_is_supported(void) {
    return msr_supported;
}

u64 msr_get_tsc(void) {
    u32 low, high;
    __asm__ volatile("rdtsc" : "=a"(low), "=d"(high));
    return ((u64)high << 32) | low;
}

u64 msr_get_tsc_frequency(void) {
    /* Try to determine TSC frequency from various sources */
    
    /* Method 1: Use CPUID if available */
    u32 eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x15));
    
    if (eax != 0 && ebx != 0) {
        /* TSC frequency = (ecx * ebx) / eax */
        if (ecx != 0) {
            return ((u64)ecx * ebx) / eax;
        }
    }
    
    /* Method 2: Use platform info MSR (Intel) */
    if (msr_supported) {
        u64 platform_info = msr_read(MSR_PLATFORM_ID);
        if (platform_info != 0) {
            /* Extract base frequency */
            u32 base_freq = (platform_info >> 8) & 0xFF;
            if (base_freq != 0) {
                return base_freq * 100000000ULL; /* Base freq in 100MHz units */
            }
        }
    }
    
    /* Method 3: Calibrate using PIT (fallback) */
    extern void timer_delay(u32 ms);
    
    u64 tsc_start = msr_get_tsc();
    timer_delay(100); /* 100ms */
    u64 tsc_end = msr_get_tsc();
    
    return (tsc_end - tsc_start) * 10; /* Extrapolate to 1 second */
}

void msr_enable_apic(u64 apic_base) {
    if (!msr_supported) return;
    
    u64 apic_msr = msr_read(MSR_APIC_BASE);
    apic_msr |= (1 << 11); /* Global enable */
    apic_msr = (apic_msr & 0xFFFFF000) | (apic_base & 0xFFFFF000);
    
    msr_write(MSR_APIC_BASE, apic_msr);
}

u64 msr_get_apic_base(void) {
    if (!msr_supported) return 0;
    
    u64 apic_msr = msr_read(MSR_APIC_BASE);
    return apic_msr & 0xFFFFF000;
}

void msr_set_pat(u64 pat_value) {
    if (!msr_supported) return;
    
    msr_write(MSR_PAT, pat_value);
}

u64 msr_get_pat(void) {
    if (!msr_supported) return 0;
    
    return msr_read(MSR_PAT);
}

void msr_enable_syscall(u64 star, u64 lstar, u64 cstar, u64 mask) {
    if (!msr_supported) return;
    
    /* Enable SYSCALL/SYSRET in EFER */
    u64 efer = msr_read(MSR_EFER);
    efer |= (1 << 0); /* SCE bit */
    msr_write(MSR_EFER, efer);
    
    /* Set up SYSCALL MSRs */
    msr_write(MSR_STAR, star);
    msr_write(MSR_LSTAR, lstar);
    msr_write(MSR_CSTAR, cstar);
    msr_write(MSR_SYSCALL_MASK, mask);
}

void msr_set_fs_base(u64 base) {
    if (!msr_supported) return;
    
    msr_write(MSR_FS_BASE, base);
}

void msr_set_gs_base(u64 base) {
    if (!msr_supported) return;
    
    msr_write(MSR_GS_BASE, base);
}

void msr_set_kernel_gs_base(u64 base) {
    if (!msr_supported) return;
    
    msr_write(MSR_KERNEL_GS_BASE, base);
}

u64 msr_get_fs_base(void) {
    if (!msr_supported) return 0;
    
    return msr_read(MSR_FS_BASE);
}

u64 msr_get_gs_base(void) {
    if (!msr_supported) return 0;
    
    return msr_read(MSR_GS_BASE);
}

u64 msr_get_kernel_gs_base(void) {
    if (!msr_supported) return 0;
    
    return msr_read(MSR_KERNEL_GS_BASE);
}

void msr_setup_mtrr(u8 reg, u64 base, u64 mask, u8 type) {
    if (!msr_supported || reg >= 8) return;
    
    u32 base_msr = MSR_MTRRPHYSBASE0 + (reg * 2);
    u32 mask_msr = MSR_MTRRPHYSMASK0 + (reg * 2);
    
    msr_write(base_msr, (base & 0xFFFFF000) | (type & 0xFF));
    msr_write(mask_msr, (mask & 0xFFFFF000) | (1 << 11)); /* Valid bit */
}

void msr_enable_mtrr(void) {
    if (!msr_supported) return;
    
    u64 deftype = msr_read(MSR_MTRRDEFTYPE);
    deftype |= (1 << 11); /* Enable MTRRs */
    msr_write(MSR_MTRRDEFTYPE, deftype);
}

u32 msr_get_microcode_version(void) {
    if (!msr_supported) return 0;
    
    /* Write 0 to trigger microcode version read */
    msr_write(MSR_BIOS_SIGN_ID, 0);
    
    /* Execute CPUID to load version */
    u32 eax;
    __asm__ volatile("cpuid" : "=a"(eax) : "a"(1) : "ebx", "ecx", "edx");
    
    /* Read microcode version */
    u64 version = msr_read(MSR_BIOS_SIGN_ID);
    return (u32)(version >> 32);
}

void msr_enable_performance_monitoring(void) {
    if (!msr_supported) return;
    
    /* Enable performance counters */
    u64 perf_global_ctrl = msr_read(MSR_PERF_GLOBAL_CTRL);
    perf_global_ctrl |= 0x0F; /* Enable PMC0-3 */
    perf_global_ctrl |= 0x700000000ULL; /* Enable fixed counters */
    msr_write(MSR_PERF_GLOBAL_CTRL, perf_global_ctrl);
}

void msr_setup_performance_counter(u8 counter, u64 event_select) {
    if (!msr_supported || counter >= 4) return;
    
    u32 perfevtsel_msr = MSR_PERFEVTSEL0 + counter;
    msr_write(perfevtsel_msr, event_select);
}

u64 msr_read_performance_counter(u8 counter) {
    if (!msr_supported || counter >= 4) return 0;
    
    u32 pmc_msr = MSR_PMC0 + counter;
    return msr_read(pmc_msr);
}
