/*
 * security_ext.c – x86 CPU security features (MPX/CET extensions)
 */

#include "kernel/types.h"

/* MPX MSRs */
#define MSR_BNDCFGS                 0xD90

/* MPX BNDCFGS bits */
#define BNDCFGS_EN                  (1 << 0)
#define BNDCFGS_BNDPRESERVE         (1 << 1)

/* CET MSRs */
#define MSR_U_CET                   0x6A0
#define MSR_S_CET                   0x6A2
#define MSR_PL0_SSP                 0x6A4
#define MSR_PL1_SSP                 0x6A5
#define MSR_PL2_SSP                 0x6A6
#define MSR_PL3_SSP                 0x6A7
#define MSR_INTERRUPT_SSP_TABLE_ADDR 0x6A8

/* CET control bits */
#define CET_SH_STK_EN               (1 << 0)
#define CET_WR_SHSTK_EN             (1 << 1)
#define CET_ENDBR_EN                (1 << 2)
#define CET_LEG_IW_EN               (1 << 3)
#define CET_NO_TRACK_EN             (1 << 4)
#define CET_SUPPRESS_DIS            (1 << 5)
#define CET_SUPPRESS                (1 << 10)
#define CET_TRACKER                 (1 << 11)

/* Control register bits */
#define CR4_SMEP                    (1 << 20)
#define CR4_SMAP                    (1 << 21)
#define CR4_PKE                     (1 << 22)
#define CR4_CET                     (1 << 23)

/* EFER bits */
#define EFER_NXE                    (1 << 11)

/* Security violation types */
#define SECURITY_VIOLATION_MPX      1
#define SECURITY_VIOLATION_CET_SS   2
#define SECURITY_VIOLATION_CET_IBT  3
#define SECURITY_VIOLATION_SMEP     4
#define SECURITY_VIOLATION_SMAP     5
#define SECURITY_VIOLATION_PKU      6

typedef struct {
    u64 lower_bound;
    u64 upper_bound;
    u8 enabled;
    u8 preserve;
} mpx_bound_t;

typedef struct {
    u32 violation_type;
    u64 fault_address;
    u64 instruction_pointer;
    u32 error_code;
    u64 timestamp;
} security_violation_t;

typedef struct {
    u8 security_ext_supported;
    u8 mpx_supported;
    u8 cet_supported;
    u8 cet_ss_supported;
    u8 cet_ibt_supported;
    u8 mpx_enabled;
    u8 cet_enabled;
    u8 shadow_stack_enabled;
    u8 indirect_branch_tracking_enabled;
    u8 nx_enabled;
    mpx_bound_t bounds[4];
    u64 shadow_stack_base;
    u64 shadow_stack_size;
    u64 shadow_stack_pointer;
    u64 interrupt_ssp_table;
    u32 violation_count;
    security_violation_t violations[64];
    u32 mpx_violations;
    u32 cet_violations;
    u32 smep_violations;
    u32 smap_violations;
} security_ext_info_t;

static security_ext_info_t security_ext_info;

extern u64 msr_read(u32 msr);
extern void msr_write(u32 msr, u64 value);
extern u8 msr_is_supported(void);
extern u64 timer_get_ticks(void);

static void security_ext_detect_capabilities(void) {
    u32 eax, ebx, ecx, edx;
    
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    
    security_ext_info.mpx_supported = (ebx & (1 << 14)) != 0;
    security_ext_info.cet_supported = (ecx & (1 << 7)) != 0;
    security_ext_info.cet_ss_supported = (ecx & (1 << 7)) != 0;
    security_ext_info.cet_ibt_supported = (edx & (1 << 20)) != 0;
    
    if (security_ext_info.mpx_supported || security_ext_info.cet_supported) {
        security_ext_info.security_ext_supported = 1;
    }
}

static void security_ext_enable_nx(void) {
    if (!msr_is_supported()) return;
    
    u64 efer = msr_read(0xC0000080);
    efer |= EFER_NXE;
    msr_write(0xC0000080, efer);
    
    security_ext_info.nx_enabled = 1;
}

static void security_ext_enable_mpx(void) {
    if (!security_ext_info.mpx_supported || !msr_is_supported()) return;
    
    u64 bndcfgs = BNDCFGS_EN;
    msr_write(MSR_BNDCFGS, bndcfgs);
    
    __asm__ volatile("bndmk (%0), %%bnd0" : : "r"(0x1000));
    __asm__ volatile("bndmk (%0), %%bnd1" : : "r"(0x2000));
    __asm__ volatile("bndmk (%0), %%bnd2" : : "r"(0x3000));
    __asm__ volatile("bndmk (%0), %%bnd3" : : "r"(0x4000));
    
    security_ext_info.mpx_enabled = 1;
}

static void security_ext_enable_cet(void) {
    if (!security_ext_info.cet_supported || !msr_is_supported()) return;
    
    u32 cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= CR4_CET;
    __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));
    
    u64 s_cet = 0;
    if (security_ext_info.cet_ss_supported) {
        s_cet |= CET_SH_STK_EN | CET_WR_SHSTK_EN;
        security_ext_info.shadow_stack_enabled = 1;
    }
    
    if (security_ext_info.cet_ibt_supported) {
        s_cet |= CET_ENDBR_EN;
        security_ext_info.indirect_branch_tracking_enabled = 1;
    }
    
    msr_write(MSR_S_CET, s_cet);
    security_ext_info.cet_enabled = 1;
}

void security_ext_init(void) {
    security_ext_info.security_ext_supported = 0;
    security_ext_info.mpx_supported = 0;
    security_ext_info.cet_supported = 0;
    security_ext_info.cet_ss_supported = 0;
    security_ext_info.cet_ibt_supported = 0;
    security_ext_info.mpx_enabled = 0;
    security_ext_info.cet_enabled = 0;
    security_ext_info.shadow_stack_enabled = 0;
    security_ext_info.indirect_branch_tracking_enabled = 0;
    security_ext_info.nx_enabled = 0;
    security_ext_info.shadow_stack_base = 0;
    security_ext_info.shadow_stack_size = 0;
    security_ext_info.shadow_stack_pointer = 0;
    security_ext_info.interrupt_ssp_table = 0;
    security_ext_info.violation_count = 0;
    security_ext_info.mpx_violations = 0;
    security_ext_info.cet_violations = 0;
    security_ext_info.smep_violations = 0;
    security_ext_info.smap_violations = 0;
    
    for (u8 i = 0; i < 4; i++) {
        security_ext_info.bounds[i].lower_bound = 0;
        security_ext_info.bounds[i].upper_bound = 0;
        security_ext_info.bounds[i].enabled = 0;
        security_ext_info.bounds[i].preserve = 0;
    }
    
    for (u8 i = 0; i < 64; i++) {
        security_ext_info.violations[i].violation_type = 0;
        security_ext_info.violations[i].fault_address = 0;
        security_ext_info.violations[i].instruction_pointer = 0;
        security_ext_info.violations[i].error_code = 0;
        security_ext_info.violations[i].timestamp = 0;
    }
    
    security_ext_detect_capabilities();
    
    if (security_ext_info.security_ext_supported) {
        security_ext_enable_nx();
        security_ext_enable_mpx();
        security_ext_enable_cet();
    }
}

u8 security_ext_is_supported(void) {
    return security_ext_info.security_ext_supported;
}

u8 security_ext_is_mpx_supported(void) {
    return security_ext_info.mpx_supported;
}

u8 security_ext_is_cet_supported(void) {
    return security_ext_info.cet_supported;
}

u8 security_ext_is_mpx_enabled(void) {
    return security_ext_info.mpx_enabled;
}

u8 security_ext_is_cet_enabled(void) {
    return security_ext_info.cet_enabled;
}

u8 security_ext_is_shadow_stack_enabled(void) {
    return security_ext_info.shadow_stack_enabled;
}

u8 security_ext_is_ibt_enabled(void) {
    return security_ext_info.indirect_branch_tracking_enabled;
}

u8 security_ext_is_nx_enabled(void) {
    return security_ext_info.nx_enabled;
}

void security_ext_setup_shadow_stack(u64 base, u64 size) {
    if (!security_ext_info.shadow_stack_enabled || !msr_is_supported()) return;
    
    security_ext_info.shadow_stack_base = base;
    security_ext_info.shadow_stack_size = size;
    security_ext_info.shadow_stack_pointer = base + size;
    
    msr_write(MSR_PL0_SSP, security_ext_info.shadow_stack_pointer);
}

u64 security_ext_get_shadow_stack_pointer(void) {
    if (!security_ext_info.shadow_stack_enabled || !msr_is_supported()) return 0;
    
    return msr_read(MSR_PL0_SSP);
}

void security_ext_setup_interrupt_ssp_table(u64 table_address) {
    if (!security_ext_info.shadow_stack_enabled || !msr_is_supported()) return;
    
    security_ext_info.interrupt_ssp_table = table_address;
    msr_write(MSR_INTERRUPT_SSP_TABLE_ADDR, table_address);
}

void security_ext_set_mpx_bound(u8 index, u64 lower, u64 upper) {
    if (!security_ext_info.mpx_enabled || index >= 4) return;
    
    security_ext_info.bounds[index].lower_bound = lower;
    security_ext_info.bounds[index].upper_bound = upper;
    security_ext_info.bounds[index].enabled = 1;
    
    switch (index) {
        case 0:
            __asm__ volatile("bndmk (%0), %%bnd0" : : "r"(lower));
            break;
        case 1:
            __asm__ volatile("bndmk (%0), %%bnd1" : : "r"(lower));
            break;
        case 2:
            __asm__ volatile("bndmk (%0), %%bnd2" : : "r"(lower));
            break;
        case 3:
            __asm__ volatile("bndmk (%0), %%bnd3" : : "r"(lower));
            break;
    }
}

const mpx_bound_t* security_ext_get_mpx_bound(u8 index) {
    if (index >= 4) return 0;
    return &security_ext_info.bounds[index];
}

void security_ext_clear_mpx_bound(u8 index) {
    if (!security_ext_info.mpx_enabled || index >= 4) return;
    
    security_ext_info.bounds[index].enabled = 0;
    security_ext_info.bounds[index].lower_bound = 0;
    security_ext_info.bounds[index].upper_bound = 0;
    
    switch (index) {
        case 0:
            __asm__ volatile("bndcl %0, %%bnd0" : : "r"(0));
            break;
        case 1:
            __asm__ volatile("bndcl %0, %%bnd1" : : "r"(0));
            break;
        case 2:
            __asm__ volatile("bndcl %0, %%bnd2" : : "r"(0));
            break;
        case 3:
            __asm__ volatile("bndcl %0, %%bnd3" : : "r"(0));
            break;
    }
}

void security_ext_endbr64(void) {
    if (!security_ext_info.indirect_branch_tracking_enabled) return;
    
    __asm__ volatile("endbr64");
}

void security_ext_endbr32(void) {
    if (!security_ext_info.indirect_branch_tracking_enabled) return;
    
    __asm__ volatile("endbr32");
}

void security_ext_handle_violation(u32 violation_type, u64 fault_address, u64 rip, u32 error_code) {
    if (security_ext_info.violation_count >= 64) return;
    
    security_violation_t* violation = &security_ext_info.violations[security_ext_info.violation_count];
    
    violation->violation_type = violation_type;
    violation->fault_address = fault_address;
    violation->instruction_pointer = rip;
    violation->error_code = error_code;
    violation->timestamp = timer_get_ticks();
    
    security_ext_info.violation_count++;
    
    switch (violation_type) {
        case SECURITY_VIOLATION_MPX:
            security_ext_info.mpx_violations++;
            break;
        case SECURITY_VIOLATION_CET_SS:
        case SECURITY_VIOLATION_CET_IBT:
            security_ext_info.cet_violations++;
            break;
        case SECURITY_VIOLATION_SMEP:
            security_ext_info.smep_violations++;
            break;
        case SECURITY_VIOLATION_SMAP:
            security_ext_info.smap_violations++;
            break;
    }
}

u32 security_ext_get_violation_count(void) {
    return security_ext_info.violation_count;
}

const security_violation_t* security_ext_get_violation(u32 index) {
    if (index >= security_ext_info.violation_count || index >= 64) return 0;
    return &security_ext_info.violations[index];
}

u32 security_ext_get_mpx_violations(void) {
    return security_ext_info.mpx_violations;
}

u32 security_ext_get_cet_violations(void) {
    return security_ext_info.cet_violations;
}

u32 security_ext_get_smep_violations(void) {
    return security_ext_info.smep_violations;
}

u32 security_ext_get_smap_violations(void) {
    return security_ext_info.smap_violations;
}

void security_ext_clear_violations(void) {
    security_ext_info.violation_count = 0;
    security_ext_info.mpx_violations = 0;
    security_ext_info.cet_violations = 0;
    security_ext_info.smep_violations = 0;
    security_ext_info.smap_violations = 0;
}

void security_ext_disable_mpx(void) {
    if (!security_ext_info.mpx_enabled || !msr_is_supported()) return;
    
    msr_write(MSR_BNDCFGS, 0);
    security_ext_info.mpx_enabled = 0;
    
    for (u8 i = 0; i < 4; i++) {
        security_ext_info.bounds[i].enabled = 0;
    }
}

void security_ext_disable_cet(void) {
    if (!security_ext_info.cet_enabled || !msr_is_supported()) return;
    
    msr_write(MSR_S_CET, 0);
    msr_write(MSR_U_CET, 0);
    
    security_ext_info.cet_enabled = 0;
    security_ext_info.shadow_stack_enabled = 0;
    security_ext_info.indirect_branch_tracking_enabled = 0;
}

u8 security_ext_check_mpx_bounds(u64 address, u8 bound_index) {
    if (!security_ext_info.mpx_enabled || bound_index >= 4) return 0;
    
    const mpx_bound_t* bound = &security_ext_info.bounds[bound_index];
    if (!bound->enabled) return 1;
    
    return (address >= bound->lower_bound && address <= bound->upper_bound);
}

void security_ext_set_mpx_preserve(u8 preserve) {
    if (!security_ext_info.mpx_enabled || !msr_is_supported()) return;
    
    u64 bndcfgs = msr_read(MSR_BNDCFGS);
    if (preserve) {
        bndcfgs |= BNDCFGS_BNDPRESERVE;
    } else {
        bndcfgs &= ~BNDCFGS_BNDPRESERVE;
    }
    msr_write(MSR_BNDCFGS, bndcfgs);
    
    for (u8 i = 0; i < 4; i++) {
        security_ext_info.bounds[i].preserve = preserve;
    }
}

u8 security_ext_is_mpx_preserve_enabled(void) {
    if (!security_ext_info.mpx_enabled || !msr_is_supported()) return 0;
    
    u64 bndcfgs = msr_read(MSR_BNDCFGS);
    return (bndcfgs & BNDCFGS_BNDPRESERVE) != 0;
}

u64 security_ext_get_shadow_stack_base(void) {
    return security_ext_info.shadow_stack_base;
}

u64 security_ext_get_shadow_stack_size(void) {
    return security_ext_info.shadow_stack_size;
}

u64 security_ext_get_interrupt_ssp_table(void) {
    return security_ext_info.interrupt_ssp_table;
}
