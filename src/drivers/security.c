/*
 * security.c – x86 CPU security features (CET, SMEP/SMAP, MPX)
 */

#include "kernel/types.h"

/* Control Flow Enforcement Technology MSRs */
#define MSR_U_CET               0x6A0
#define MSR_S_CET               0x6A2
#define MSR_PL0_SSP             0x6A4
#define MSR_PL1_SSP             0x6A5
#define MSR_PL2_SSP             0x6A6
#define MSR_PL3_SSP             0x6A7
#define MSR_INTERRUPT_SSP_TABLE 0x6A8

/* CET control bits */
#define CET_SH_STK_EN           0x01    /* Shadow stack enable */
#define CET_WR_SHSTK_EN         0x02    /* Writable shadow stack */
#define CET_ENDBR_EN            0x04    /* ENDBRANCH enable */
#define CET_LEG_IW_EN           0x08    /* Legacy indirect branch */
#define CET_NO_TRACK_EN         0x10    /* No track enable */
#define CET_SUPPRESS_DIS        0x20    /* Suppress disable */
#define CET_SUPPRESS            0x400   /* Suppress */
#define CET_TRACKER             0x800   /* Tracker */

/* CR4 security bits */
#define CR4_SMEP                0x100000  /* Supervisor Mode Execution Prevention */
#define CR4_SMAP                0x200000  /* Supervisor Mode Access Prevention */
#define CR4_PKE                 0x400000  /* Protection Key Enable */
#define CR4_CET                 0x800000  /* Control Flow Enforcement */

/* MPX MSRs */
#define MSR_BNDCFGS             0xD90

/* MPX BNDCFGS bits */
#define BNDCFGS_EN              0x01    /* MPX enable */
#define BNDCFGS_BNDPRESERVE     0x02    /* Preserve bounds on init */

/* Protection Key Rights MSR */
#define MSR_PKRU                0x6C

typedef struct {
    u8 smep_supported;
    u8 smap_supported;
    u8 cet_supported;
    u8 cet_ss_supported;
    u8 cet_ibt_supported;
    u8 mpx_supported;
    u8 pku_supported;
    u8 smep_enabled;
    u8 smap_enabled;
    u8 cet_enabled;
    u8 mpx_enabled;
    u8 pku_enabled;
    u64 user_cet_config;
    u64 supervisor_cet_config;
    u64 mpx_config;
    u32 pkru_value;
} security_info_t;

static security_info_t security_info;

extern u64 msr_read(u32 msr);
extern void msr_write(u32 msr, u64 value);
extern u8 msr_is_supported(void);

static u8 security_check_smep_smap(void) {
    u32 eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    
    security_info.smep_supported = (ebx & (1 << 7)) != 0;
    security_info.smap_supported = (ebx & (1 << 20)) != 0;
    
    return security_info.smep_supported || security_info.smap_supported;
}

static u8 security_check_cet(void) {
    u32 eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    
    security_info.cet_supported = (ecx & (1 << 7)) != 0;
    security_info.cet_ss_supported = (edx & (1 << 18)) != 0;
    security_info.cet_ibt_supported = (edx & (1 << 20)) != 0;
    
    return security_info.cet_supported;
}

static u8 security_check_mpx(void) {
    u32 eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    
    security_info.mpx_supported = (ebx & (1 << 14)) != 0;
    
    return security_info.mpx_supported;
}

static u8 security_check_pku(void) {
    u32 eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    
    security_info.pku_supported = (ecx & (1 << 3)) != 0;
    
    return security_info.pku_supported;
}

void security_init(void) {
    security_info.smep_supported = 0;
    security_info.smap_supported = 0;
    security_info.cet_supported = 0;
    security_info.cet_ss_supported = 0;
    security_info.cet_ibt_supported = 0;
    security_info.mpx_supported = 0;
    security_info.pku_supported = 0;
    security_info.smep_enabled = 0;
    security_info.smap_enabled = 0;
    security_info.cet_enabled = 0;
    security_info.mpx_enabled = 0;
    security_info.pku_enabled = 0;
    
    security_check_smep_smap();
    security_check_cet();
    security_check_mpx();
    security_check_pku();
    
    /* Enable SMEP if supported */
    if (security_info.smep_supported) {
        u32 cr4;
        __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
        cr4 |= CR4_SMEP;
        __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));
        security_info.smep_enabled = 1;
    }
    
    /* Enable SMAP if supported */
    if (security_info.smap_supported) {
        u32 cr4;
        __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
        cr4 |= CR4_SMAP;
        __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));
        security_info.smap_enabled = 1;
    }
    
    /* Initialize PKU if supported */
    if (security_info.pku_supported) {
        u32 cr4;
        __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
        cr4 |= CR4_PKE;
        __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));
        
        /* Set default PKRU value (all keys accessible) */
        security_info.pkru_value = 0;
        __asm__ volatile("xor %%ecx, %%ecx; xor %%edx, %%edx; wrpkru" : : "a"(security_info.pkru_value));
        security_info.pku_enabled = 1;
    }
}

u8 security_is_smep_supported(void) {
    return security_info.smep_supported;
}

u8 security_is_smap_supported(void) {
    return security_info.smap_supported;
}

u8 security_is_cet_supported(void) {
    return security_info.cet_supported;
}

u8 security_is_mpx_supported(void) {
    return security_info.mpx_supported;
}

u8 security_is_pku_supported(void) {
    return security_info.pku_supported;
}

u8 security_is_smep_enabled(void) {
    return security_info.smep_enabled;
}

u8 security_is_smap_enabled(void) {
    return security_info.smap_enabled;
}

u8 security_is_cet_enabled(void) {
    return security_info.cet_enabled;
}

u8 security_is_mpx_enabled(void) {
    return security_info.mpx_enabled;
}

u8 security_is_pku_enabled(void) {
    return security_info.pku_enabled;
}

void security_enable_cet(u8 enable_shadow_stack, u8 enable_ibt) {
    if (!security_info.cet_supported || !msr_is_supported()) return;
    
    u32 cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= CR4_CET;
    __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));
    
    /* Configure supervisor CET */
    u64 s_cet = 0;
    if (enable_shadow_stack && security_info.cet_ss_supported) {
        s_cet |= CET_SH_STK_EN;
    }
    if (enable_ibt && security_info.cet_ibt_supported) {
        s_cet |= CET_ENDBR_EN;
    }
    
    if (s_cet != 0) {
        msr_write(MSR_S_CET, s_cet);
        security_info.supervisor_cet_config = s_cet;
        security_info.cet_enabled = 1;
    }
    
    /* Configure user CET */
    u64 u_cet = s_cet;
    msr_write(MSR_U_CET, u_cet);
    security_info.user_cet_config = u_cet;
}

void security_disable_cet(void) {
    if (!security_info.cet_enabled || !msr_is_supported()) return;
    
    msr_write(MSR_S_CET, 0);
    msr_write(MSR_U_CET, 0);
    
    security_info.cet_enabled = 0;
    security_info.supervisor_cet_config = 0;
    security_info.user_cet_config = 0;
}

void security_enable_mpx(void) {
    if (!security_info.mpx_supported || !msr_is_supported()) return;
    
    extern u8 xsave_enable_feature(u32 feature_bit);
    
    /* Enable MPX in XSAVE */
    if (!xsave_enable_feature(3)) return; /* BNDREGS */
    if (!xsave_enable_feature(4)) return; /* BNDCSR */
    
    /* Configure BNDCFGS */
    u64 bndcfgs = BNDCFGS_EN;
    msr_write(MSR_BNDCFGS, bndcfgs);
    
    security_info.mpx_config = bndcfgs;
    security_info.mpx_enabled = 1;
}

void security_disable_mpx(void) {
    if (!security_info.mpx_enabled || !msr_is_supported()) return;
    
    msr_write(MSR_BNDCFGS, 0);
    
    extern u8 xsave_disable_feature(u32 feature_bit);
    xsave_disable_feature(3); /* BNDREGS */
    xsave_disable_feature(4); /* BNDCSR */
    
    security_info.mpx_enabled = 0;
    security_info.mpx_config = 0;
}

void security_set_protection_key(u8 key, u8 access_disable, u8 write_disable) {
    if (!security_info.pku_enabled || key >= 16) return;
    
    u32 pkru = security_info.pkru_value;
    
    /* Clear existing bits for this key */
    pkru &= ~(3 << (key * 2));
    
    /* Set new bits */
    if (access_disable) pkru |= (1 << (key * 2));
    if (write_disable) pkru |= (2 << (key * 2));
    
    /* Update PKRU register */
    __asm__ volatile("xor %%ecx, %%ecx; xor %%edx, %%edx; wrpkru" : : "a"(pkru));
    
    security_info.pkru_value = pkru;
}

u32 security_get_pkru(void) {
    if (!security_info.pku_enabled) return 0;
    
    u32 pkru;
    __asm__ volatile("xor %%ecx, %%ecx; rdpkru" : "=a"(pkru) : : "edx");
    
    security_info.pkru_value = pkru;
    return pkru;
}

void security_stac(void) {
    if (!security_info.smap_enabled) return;
    
    __asm__ volatile("stac");
}

void security_clac(void) {
    if (!security_info.smap_enabled) return;
    
    __asm__ volatile("clac");
}

u64 security_get_supervisor_cet_config(void) {
    return security_info.supervisor_cet_config;
}

u64 security_get_user_cet_config(void) {
    return security_info.user_cet_config;
}

u64 security_get_mpx_config(void) {
    return security_info.mpx_config;
}

void security_setup_shadow_stack(u64 ssp_base, u32 size) {
    if (!security_info.cet_enabled || !security_info.cet_ss_supported) return;
    if (!msr_is_supported()) return;
    
    /* Setup supervisor shadow stack pointer */
    msr_write(MSR_PL0_SSP, ssp_base + size - 8);
}

u8 security_is_shadow_stack_supported(void) {
    return security_info.cet_ss_supported;
}

u8 security_is_ibt_supported(void) {
    return security_info.cet_ibt_supported;
}

void security_handle_cet_violation(u32 error_code) {
    /* CET violation handler */
    if (error_code & 1) {
        /* Shadow stack violation */
    }
    if (error_code & 2) {
        /* Indirect branch tracking violation */
    }
    
    /* Log violation and potentially terminate process */
}

void security_handle_mpx_violation(void) {
    /* MPX bounds violation handler */
    
    /* Read bound registers and handle violation */
}

u8 security_validate_indirect_branch(u64 target_address) {
    if (!security_info.cet_enabled || !security_info.cet_ibt_supported) return 1;
    
    /* Check if target starts with ENDBR instruction */
    u32* target = (u32*)target_address;
    
    /* ENDBR64: F3 0F 1E FA */
    /* ENDBR32: F3 0F 1E FB */
    if (*target == 0xFA1E0FF3 || *target == 0xFB1E0FF3) {
        return 1;
    }
    
    return 0;
}
