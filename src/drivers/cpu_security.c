/*
 * cpu_security.c – x86 CPU security features (CET/MPX/SMEP/SMAP/PKU)
 */

#include "kernel/types.h"

/* Control Flow Enforcement Technology (CET) MSRs */
#define MSR_U_CET                       0x6A0
#define MSR_S_CET                       0x6A2
#define MSR_PL0_SSP                     0x6A4
#define MSR_PL1_SSP                     0x6A5
#define MSR_PL2_SSP                     0x6A6
#define MSR_PL3_SSP                     0x6A7
#define MSR_INTERRUPT_SSP_TABLE_ADDR    0x6A8

/* CET control bits */
#define CET_SH_STK_EN                   (1ULL << 0)
#define CET_WR_SHSTK_EN                 (1ULL << 1)
#define CET_ENDBR_EN                    (1ULL << 2)
#define CET_LEG_IW_EN                   (1ULL << 3)
#define CET_NO_TRACK_EN                 (1ULL << 4)
#define CET_SUPPRESS_DIS                (1ULL << 5)
#define CET_SUPPRESS                    (1ULL << 10)
#define CET_TRACKER                     (1ULL << 11)

/* Memory Protection Extensions (MPX) MSRs */
#define MSR_BNDCFGS                     0xD90

/* BNDCFGS bits */
#define BNDCFGS_EN                      (1ULL << 0)
#define BNDCFGS_BNDPRESERVE             (1ULL << 1)

/* Protection Keys MSRs */
#define MSR_PKRU                        0x6C0

/* CR4 security bits */
#define CR4_SMEP                        (1ULL << 20)
#define CR4_SMAP                        (1ULL << 21)
#define CR4_PKE                         (1ULL << 22)
#define CR4_CET                         (1ULL << 23)
#define CR4_PKS                         (1ULL << 24)

/* XCR0 bits for security features */
#define XCR0_BNDREGS                    (1ULL << 3)
#define XCR0_BNDCSR                     (1ULL << 4)
#define XCR0_CET_U                      (1ULL << 11)
#define XCR0_CET_S                      (1ULL << 12)

typedef struct {
    u8 cet_supported;
    u8 cet_ss_supported;
    u8 cet_ibt_supported;
    u8 cet_user_enabled;
    u8 cet_supervisor_enabled;
    u64 user_cet_config;
    u64 supervisor_cet_config;
    u64 shadow_stack_pointers[4];
} cet_info_t;

typedef struct {
    u8 mpx_supported;
    u8 mpx_enabled;
    u64 bndcfgs_value;
    u32 bound_violations;
    u64 last_violation_address;
} mpx_info_t;

typedef struct {
    u8 pku_supported;
    u8 pks_supported;
    u8 pku_enabled;
    u8 pks_enabled;
    u32 pkru_value;
    u64 pkrs_value;
    u32 protection_key_violations;
    u64 last_pkey_violation;
} protection_keys_info_t;

typedef struct {
    u8 cpu_security_supported;
    u8 smep_supported;
    u8 smap_supported;
    u8 smep_enabled;
    u8 smap_enabled;
    cet_info_t cet;
    mpx_info_t mpx;
    protection_keys_info_t pkeys;
    u32 total_security_violations;
    u32 smep_violations;
    u32 smap_violations;
    u64 last_security_event;
    u32 security_mitigations_active;
} cpu_security_info_t;

static cpu_security_info_t cpu_security_info;

extern u64 msr_read(u32 msr);
extern void msr_write(u32 msr, u64 value);
extern u8 msr_is_supported(void);
extern u64 timer_get_ticks(void);

static void cpu_security_detect_cet(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check for CET support */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    
    cpu_security_info.cet.cet_supported = (ecx & (1 << 7)) != 0;
    cpu_security_info.cet.cet_ss_supported = (edx & (1 << 18)) != 0;
    cpu_security_info.cet.cet_ibt_supported = (edx & (1 << 20)) != 0;
    
    if (cpu_security_info.cet.cet_supported) {
        cpu_security_info.security_mitigations_active++;
    }
}

static void cpu_security_detect_mpx(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check for MPX support */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    
    cpu_security_info.mpx.mpx_supported = (ebx & (1 << 14)) != 0;
    
    if (cpu_security_info.mpx.mpx_supported) {
        cpu_security_info.security_mitigations_active++;
    }
}

static void cpu_security_detect_protection_keys(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check for Protection Keys support */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    
    cpu_security_info.pkeys.pku_supported = (ecx & (1 << 3)) != 0;
    cpu_security_info.pkeys.pks_supported = (ecx & (1 << 31)) != 0;
    
    if (cpu_security_info.pkeys.pku_supported || cpu_security_info.pkeys.pks_supported) {
        cpu_security_info.security_mitigations_active++;
    }
}

static void cpu_security_detect_smep_smap(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check for SMEP/SMAP support */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    
    cpu_security_info.smep_supported = (ebx & (1 << 7)) != 0;
    cpu_security_info.smap_supported = (ebx & (1 << 20)) != 0;
    
    /* Check if already enabled in CR4 */
    u64 cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    
    cpu_security_info.smep_enabled = (cr4 & CR4_SMEP) != 0;
    cpu_security_info.smap_enabled = (cr4 & CR4_SMAP) != 0;
    
    if (cpu_security_info.smep_supported || cpu_security_info.smap_supported) {
        cpu_security_info.security_mitigations_active++;
    }
}

void cpu_security_init(void) {
    cpu_security_info.cpu_security_supported = 0;
    cpu_security_info.smep_supported = 0;
    cpu_security_info.smap_supported = 0;
    cpu_security_info.smep_enabled = 0;
    cpu_security_info.smap_enabled = 0;
    cpu_security_info.total_security_violations = 0;
    cpu_security_info.smep_violations = 0;
    cpu_security_info.smap_violations = 0;
    cpu_security_info.last_security_event = 0;
    cpu_security_info.security_mitigations_active = 0;
    
    /* Initialize CET */
    cpu_security_info.cet.cet_supported = 0;
    cpu_security_info.cet.cet_ss_supported = 0;
    cpu_security_info.cet.cet_ibt_supported = 0;
    cpu_security_info.cet.cet_user_enabled = 0;
    cpu_security_info.cet.cet_supervisor_enabled = 0;
    cpu_security_info.cet.user_cet_config = 0;
    cpu_security_info.cet.supervisor_cet_config = 0;
    
    for (u8 i = 0; i < 4; i++) {
        cpu_security_info.cet.shadow_stack_pointers[i] = 0;
    }
    
    /* Initialize MPX */
    cpu_security_info.mpx.mpx_supported = 0;
    cpu_security_info.mpx.mpx_enabled = 0;
    cpu_security_info.mpx.bndcfgs_value = 0;
    cpu_security_info.mpx.bound_violations = 0;
    cpu_security_info.mpx.last_violation_address = 0;
    
    /* Initialize Protection Keys */
    cpu_security_info.pkeys.pku_supported = 0;
    cpu_security_info.pkeys.pks_supported = 0;
    cpu_security_info.pkeys.pku_enabled = 0;
    cpu_security_info.pkeys.pks_enabled = 0;
    cpu_security_info.pkeys.pkru_value = 0;
    cpu_security_info.pkeys.pkrs_value = 0;
    cpu_security_info.pkeys.protection_key_violations = 0;
    cpu_security_info.pkeys.last_pkey_violation = 0;
    
    cpu_security_detect_smep_smap();
    cpu_security_detect_cet();
    cpu_security_detect_mpx();
    cpu_security_detect_protection_keys();
    
    if (cpu_security_info.smep_supported || cpu_security_info.smap_supported ||
        cpu_security_info.cet.cet_supported || cpu_security_info.mpx.mpx_supported ||
        cpu_security_info.pkeys.pku_supported) {
        cpu_security_info.cpu_security_supported = 1;
    }
}

u8 cpu_security_is_supported(void) {
    return cpu_security_info.cpu_security_supported;
}

u8 cpu_security_enable_smep(void) {
    if (!cpu_security_info.smep_supported) return 0;
    
    u64 cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= CR4_SMEP;
    __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));
    
    cpu_security_info.smep_enabled = 1;
    return 1;
}

u8 cpu_security_enable_smap(void) {
    if (!cpu_security_info.smap_supported) return 0;
    
    u64 cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= CR4_SMAP;
    __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));
    
    cpu_security_info.smap_enabled = 1;
    return 1;
}

u8 cpu_security_enable_cet_user(void) {
    if (!cpu_security_info.cet.cet_supported || !msr_is_supported()) return 0;
    
    /* Enable CET in CR4 */
    u64 cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= CR4_CET;
    __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));
    
    /* Configure user CET */
    u64 cet_config = CET_SH_STK_EN | CET_ENDBR_EN;
    msr_write(MSR_U_CET, cet_config);
    
    cpu_security_info.cet.cet_user_enabled = 1;
    cpu_security_info.cet.user_cet_config = cet_config;
    
    return 1;
}

u8 cpu_security_enable_cet_supervisor(void) {
    if (!cpu_security_info.cet.cet_supported || !msr_is_supported()) return 0;
    
    /* Configure supervisor CET */
    u64 cet_config = CET_SH_STK_EN | CET_ENDBR_EN;
    msr_write(MSR_S_CET, cet_config);
    
    cpu_security_info.cet.cet_supervisor_enabled = 1;
    cpu_security_info.cet.supervisor_cet_config = cet_config;
    
    return 1;
}

u8 cpu_security_enable_mpx(void) {
    if (!cpu_security_info.mpx.mpx_supported || !msr_is_supported()) return 0;
    
    /* Enable MPX in BNDCFGS */
    u64 bndcfgs = BNDCFGS_EN;
    msr_write(MSR_BNDCFGS, bndcfgs);
    
    /* Enable MPX state in XCR0 */
    u64 xcr0;
    __asm__ volatile("xgetbv" : "=a"((u32)xcr0), "=d"((u32)(xcr0 >> 32)) : "c"(0));
    xcr0 |= XCR0_BNDREGS | XCR0_BNDCSR;
    __asm__ volatile("xsetbv" : : "a"((u32)xcr0), "d"((u32)(xcr0 >> 32)), "c"(0));
    
    cpu_security_info.mpx.mpx_enabled = 1;
    cpu_security_info.mpx.bndcfgs_value = bndcfgs;
    
    return 1;
}

u8 cpu_security_enable_protection_keys(void) {
    if (!cpu_security_info.pkeys.pku_supported) return 0;
    
    /* Enable PKU in CR4 */
    u64 cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= CR4_PKE;
    __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));
    
    /* Initialize PKRU to allow all access */
    u32 pkru = 0;
    __asm__ volatile("wrpkru" : : "a"(pkru), "c"(0), "d"(0));
    
    cpu_security_info.pkeys.pku_enabled = 1;
    cpu_security_info.pkeys.pkru_value = pkru;
    
    return 1;
}

void cpu_security_set_protection_key(u8 key, u8 access_disable, u8 write_disable) {
    if (!cpu_security_info.pkeys.pku_enabled || key >= 16) return;
    
    u32 pkru = cpu_security_info.pkeys.pkru_value;
    
    /* Clear existing bits for this key */
    pkru &= ~(3 << (key * 2));
    
    /* Set new bits */
    if (access_disable) pkru |= (1 << (key * 2));
    if (write_disable) pkru |= (2 << (key * 2));
    
    __asm__ volatile("wrpkru" : : "a"(pkru), "c"(0), "d"(0));
    cpu_security_info.pkeys.pkru_value = pkru;
}

void cpu_security_handle_smep_violation(u64 fault_address) {
    cpu_security_info.smep_violations++;
    cpu_security_info.total_security_violations++;
    cpu_security_info.last_security_event = timer_get_ticks();
}

void cpu_security_handle_smap_violation(u64 fault_address) {
    cpu_security_info.smap_violations++;
    cpu_security_info.total_security_violations++;
    cpu_security_info.last_security_event = timer_get_ticks();
}

void cpu_security_handle_mpx_violation(u64 fault_address) {
    cpu_security_info.mpx.bound_violations++;
    cpu_security_info.mpx.last_violation_address = fault_address;
    cpu_security_info.total_security_violations++;
    cpu_security_info.last_security_event = timer_get_ticks();
}

void cpu_security_handle_pkey_violation(u64 fault_address, u8 key) {
    cpu_security_info.pkeys.protection_key_violations++;
    cpu_security_info.pkeys.last_pkey_violation = fault_address;
    cpu_security_info.total_security_violations++;
    cpu_security_info.last_security_event = timer_get_ticks();
}

u8 cpu_security_is_smep_supported(void) {
    return cpu_security_info.smep_supported;
}

u8 cpu_security_is_smap_supported(void) {
    return cpu_security_info.smap_supported;
}

u8 cpu_security_is_smep_enabled(void) {
    return cpu_security_info.smep_enabled;
}

u8 cpu_security_is_smap_enabled(void) {
    return cpu_security_info.smap_enabled;
}

u8 cpu_security_is_cet_supported(void) {
    return cpu_security_info.cet.cet_supported;
}

u8 cpu_security_is_cet_user_enabled(void) {
    return cpu_security_info.cet.cet_user_enabled;
}

u8 cpu_security_is_cet_supervisor_enabled(void) {
    return cpu_security_info.cet.cet_supervisor_enabled;
}

u8 cpu_security_is_mpx_supported(void) {
    return cpu_security_info.mpx.mpx_supported;
}

u8 cpu_security_is_mpx_enabled(void) {
    return cpu_security_info.mpx.mpx_enabled;
}

u8 cpu_security_is_pku_supported(void) {
    return cpu_security_info.pkeys.pku_supported;
}

u8 cpu_security_is_pku_enabled(void) {
    return cpu_security_info.pkeys.pku_enabled;
}

u32 cpu_security_get_total_violations(void) {
    return cpu_security_info.total_security_violations;
}

u32 cpu_security_get_smep_violations(void) {
    return cpu_security_info.smep_violations;
}

u32 cpu_security_get_smap_violations(void) {
    return cpu_security_info.smap_violations;
}

u32 cpu_security_get_mpx_violations(void) {
    return cpu_security_info.mpx.bound_violations;
}

u32 cpu_security_get_pkey_violations(void) {
    return cpu_security_info.pkeys.protection_key_violations;
}

u64 cpu_security_get_last_security_event(void) {
    return cpu_security_info.last_security_event;
}

u32 cpu_security_get_active_mitigations(void) {
    return cpu_security_info.security_mitigations_active;
}

u64 cpu_security_get_user_cet_config(void) {
    return cpu_security_info.cet.user_cet_config;
}

u64 cpu_security_get_supervisor_cet_config(void) {
    return cpu_security_info.cet.supervisor_cet_config;
}

u32 cpu_security_get_pkru_value(void) {
    return cpu_security_info.pkeys.pkru_value;
}

void cpu_security_clear_statistics(void) {
    cpu_security_info.total_security_violations = 0;
    cpu_security_info.smep_violations = 0;
    cpu_security_info.smap_violations = 0;
    cpu_security_info.mpx.bound_violations = 0;
    cpu_security_info.pkeys.protection_key_violations = 0;
}
