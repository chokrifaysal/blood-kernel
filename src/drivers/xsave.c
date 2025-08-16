/*
 * xsave.c – x86 XSAVE/XRSTOR extended state management
 */

#include "kernel/types.h"

/* XSAVE feature bits */
#define XFEATURE_FP             0x01    /* x87 floating point */
#define XFEATURE_SSE            0x02    /* SSE */
#define XFEATURE_YMM            0x04    /* AVX */
#define XFEATURE_BNDREGS        0x08    /* MPX bound registers */
#define XFEATURE_BNDCSR         0x10    /* MPX bound config/status */
#define XFEATURE_OPMASK         0x20    /* AVX-512 opmask */
#define XFEATURE_ZMM_Hi256      0x40    /* AVX-512 upper 256 bits of ZMM0-15 */
#define XFEATURE_Hi16_ZMM       0x80    /* AVX-512 ZMM16-31 */
#define XFEATURE_PT             0x100   /* Processor Trace */
#define XFEATURE_PKRU           0x200   /* Protection Key Rights */
#define XFEATURE_PASID          0x400   /* PASID state */
#define XFEATURE_CET_U          0x800   /* CET user state */
#define XFEATURE_CET_S          0x1000  /* CET supervisor state */
#define XFEATURE_HDC            0x2000  /* Hardware Duty Cycling */
#define XFEATURE_UINTR          0x4000  /* User Interrupts */
#define XFEATURE_LBR            0x8000  /* Last Branch Records */
#define XFEATURE_HWP            0x10000 /* Hardware P-states */

/* CR4 bits */
#define CR4_OSXSAVE             0x40000

/* XCR0 register */
#define XCR_XFEATURE_ENABLED_MASK 0

typedef struct {
    u8 xsave_supported;
    u8 xsaveopt_supported;
    u8 xsavec_supported;
    u8 xsaves_supported;
    u8 xfd_supported;
    u32 max_xsave_size;
    u32 max_xsave_size_enabled;
    u64 supported_features;
    u64 enabled_features;
    u32 feature_offsets[32];
    u32 feature_sizes[32];
    u8 supervisor_features;
} xsave_info_t;

static xsave_info_t xsave_info;

extern u64 msr_read(u32 msr);
extern void msr_write(u32 msr, u64 value);
extern u8 msr_is_supported(void);

static u8 xsave_check_support(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check for XSAVE support */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    
    if (!(ecx & (1 << 26))) return 0; /* XSAVE not supported */
    
    xsave_info.xsave_supported = 1;
    
    /* Check for extended XSAVE features */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0xD), "c"(1));
    
    xsave_info.xsaveopt_supported = (eax & (1 << 0)) != 0;
    xsave_info.xsavec_supported = (eax & (1 << 1)) != 0;
    xsave_info.xsaves_supported = (eax & (1 << 3)) != 0;
    xsave_info.xfd_supported = (eax & (1 << 4)) != 0;
    
    return 1;
}

static void xsave_enumerate_features(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Get main XSAVE info */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0xD), "c"(0));
    
    xsave_info.supported_features = ((u64)edx << 32) | eax;
    xsave_info.max_xsave_size = ecx;
    xsave_info.max_xsave_size_enabled = ebx;
    
    /* Enumerate individual features */
    for (u32 i = 2; i < 32; i++) {
        if (!(xsave_info.supported_features & (1ULL << i))) continue;
        
        __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0xD), "c"(i));
        
        xsave_info.feature_sizes[i] = eax;
        xsave_info.feature_offsets[i] = ebx;
        
        if (ecx & 1) {
            xsave_info.supervisor_features |= (1 << i);
        }
    }
    
    /* Set standard feature info */
    xsave_info.feature_sizes[0] = 160;  /* x87 state */
    xsave_info.feature_offsets[0] = 0;
    xsave_info.feature_sizes[1] = 256;  /* SSE state */
    xsave_info.feature_offsets[1] = 160;
}

static void xsave_enable_osxsave(void) {
    u32 cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= CR4_OSXSAVE;
    __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));
}

static void xsave_set_xcr0(u64 features) {
    u32 eax = features & 0xFFFFFFFF;
    u32 edx = features >> 32;
    
    __asm__ volatile("xsetbv" : : "a"(eax), "d"(edx), "c"(XCR_XFEATURE_ENABLED_MASK));
    
    xsave_info.enabled_features = features;
}

void xsave_init(void) {
    xsave_info.xsave_supported = 0;
    xsave_info.xsaveopt_supported = 0;
    xsave_info.xsavec_supported = 0;
    xsave_info.xsaves_supported = 0;
    xsave_info.xfd_supported = 0;
    xsave_info.supported_features = 0;
    xsave_info.enabled_features = 0;
    xsave_info.supervisor_features = 0;
    
    if (!xsave_check_support()) return;
    
    xsave_enumerate_features();
    xsave_enable_osxsave();
    
    /* Enable basic features: x87 + SSE */
    u64 basic_features = XFEATURE_FP | XFEATURE_SSE;
    
    /* Enable AVX if supported */
    if (xsave_info.supported_features & XFEATURE_YMM) {
        basic_features |= XFEATURE_YMM;
    }
    
    /* Enable AVX-512 if supported */
    if (xsave_info.supported_features & XFEATURE_OPMASK) {
        basic_features |= XFEATURE_OPMASK | XFEATURE_ZMM_Hi256 | XFEATURE_Hi16_ZMM;
    }
    
    /* Enable MPX if supported */
    if (xsave_info.supported_features & XFEATURE_BNDREGS) {
        basic_features |= XFEATURE_BNDREGS | XFEATURE_BNDCSR;
    }
    
    /* Enable Protection Keys if supported */
    if (xsave_info.supported_features & XFEATURE_PKRU) {
        basic_features |= XFEATURE_PKRU;
    }
    
    /* Only enable features that are actually supported */
    basic_features &= xsave_info.supported_features;
    
    xsave_set_xcr0(basic_features);
}

u8 xsave_is_supported(void) {
    return xsave_info.xsave_supported;
}

u8 xsave_is_xsaveopt_supported(void) {
    return xsave_info.xsaveopt_supported;
}

u8 xsave_is_xsavec_supported(void) {
    return xsave_info.xsavec_supported;
}

u8 xsave_is_xsaves_supported(void) {
    return xsave_info.xsaves_supported;
}

u64 xsave_get_supported_features(void) {
    return xsave_info.supported_features;
}

u64 xsave_get_enabled_features(void) {
    return xsave_info.enabled_features;
}

u32 xsave_get_area_size(void) {
    return xsave_info.max_xsave_size_enabled;
}

u32 xsave_get_feature_offset(u32 feature_bit) {
    if (feature_bit >= 32) return 0;
    return xsave_info.feature_offsets[feature_bit];
}

u32 xsave_get_feature_size(u32 feature_bit) {
    if (feature_bit >= 32) return 0;
    return xsave_info.feature_sizes[feature_bit];
}

void xsave_save_state(void* xsave_area, u64 feature_mask) {
    if (!xsave_info.xsave_supported || !xsave_area) return;
    
    u32 eax = feature_mask & 0xFFFFFFFF;
    u32 edx = feature_mask >> 32;
    
    if (xsave_info.xsaveopt_supported) {
        __asm__ volatile("xsaveopt (%0)" : : "r"(xsave_area), "a"(eax), "d"(edx) : "memory");
    } else {
        __asm__ volatile("xsave (%0)" : : "r"(xsave_area), "a"(eax), "d"(edx) : "memory");
    }
}

void xsave_restore_state(const void* xsave_area, u64 feature_mask) {
    if (!xsave_info.xsave_supported || !xsave_area) return;
    
    u32 eax = feature_mask & 0xFFFFFFFF;
    u32 edx = feature_mask >> 32;
    
    __asm__ volatile("xrstor (%0)" : : "r"(xsave_area), "a"(eax), "d"(edx) : "memory");
}

void xsave_save_state_compact(void* xsave_area, u64 feature_mask) {
    if (!xsave_info.xsavec_supported || !xsave_area) return;
    
    u32 eax = feature_mask & 0xFFFFFFFF;
    u32 edx = feature_mask >> 32;
    
    __asm__ volatile("xsavec (%0)" : : "r"(xsave_area), "a"(eax), "d"(edx) : "memory");
}

void xsave_save_state_supervisor(void* xsave_area, u64 feature_mask) {
    if (!xsave_info.xsaves_supported || !xsave_area) return;
    
    u32 eax = feature_mask & 0xFFFFFFFF;
    u32 edx = feature_mask >> 32;
    
    __asm__ volatile("xsaves (%0)" : : "r"(xsave_area), "a"(eax), "d"(edx) : "memory");
}

void xsave_restore_state_supervisor(const void* xsave_area, u64 feature_mask) {
    if (!xsave_info.xsaves_supported || !xsave_area) return;
    
    u32 eax = feature_mask & 0xFFFFFFFF;
    u32 edx = feature_mask >> 32;
    
    __asm__ volatile("xrstors (%0)" : : "r"(xsave_area), "a"(eax), "d"(edx) : "memory");
}

u8 xsave_is_feature_enabled(u32 feature_bit) {
    if (feature_bit >= 64) return 0;
    return (xsave_info.enabled_features & (1ULL << feature_bit)) != 0;
}

u8 xsave_enable_feature(u32 feature_bit) {
    if (feature_bit >= 64) return 0;
    if (!(xsave_info.supported_features & (1ULL << feature_bit))) return 0;
    
    u64 new_features = xsave_info.enabled_features | (1ULL << feature_bit);
    xsave_set_xcr0(new_features);
    
    return 1;
}

u8 xsave_disable_feature(u32 feature_bit) {
    if (feature_bit >= 64) return 0;
    if (feature_bit <= 1) return 0; /* Cannot disable x87 or SSE */
    
    u64 new_features = xsave_info.enabled_features & ~(1ULL << feature_bit);
    xsave_set_xcr0(new_features);
    
    return 1;
}

void* xsave_alloc_area(void) {
    if (!xsave_info.xsave_supported) return 0;
    
    extern void* paging_alloc_pages(u32 count);
    u32 pages_needed = (xsave_info.max_xsave_size_enabled + 4095) / 4096;
    
    void* area = paging_alloc_pages(pages_needed);
    if (area) {
        /* Clear the area */
        u8* ptr = (u8*)area;
        for (u32 i = 0; i < xsave_info.max_xsave_size_enabled; i++) {
            ptr[i] = 0;
        }
    }
    
    return area;
}

void xsave_init_area(void* xsave_area) {
    if (!xsave_info.xsave_supported || !xsave_area) return;
    
    /* Initialize XSAVE area with default state */
    u8* area = (u8*)xsave_area;
    
    /* Clear entire area */
    for (u32 i = 0; i < xsave_info.max_xsave_size_enabled; i++) {
        area[i] = 0;
    }
    
    /* Set up x87 default state */
    u16* fcw = (u16*)(area + 0);  /* x87 control word */
    *fcw = 0x037F;  /* Default x87 control word */
    
    /* Set up SSE default state */
    u32* mxcsr = (u32*)(area + 24);  /* MXCSR */
    *mxcsr = 0x1F80;  /* Default MXCSR value */
    
    /* Set XSTATE_BV to indicate which features are in init state */
    u64* xstate_bv = (u64*)(area + 512);
    *xstate_bv = 0;  /* All features in init state */
}

u8 xsave_is_supervisor_feature(u32 feature_bit) {
    if (feature_bit >= 32) return 0;
    return (xsave_info.supervisor_features & (1 << feature_bit)) != 0;
}
