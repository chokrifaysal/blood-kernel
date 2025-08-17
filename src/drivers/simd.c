/*
 * simd.c – x86 SIMD instruction set extensions (SSE/AVX optimization)
 */

#include "kernel/types.h"

/* SIMD instruction set support flags */
#define SIMD_SSE        0x01
#define SIMD_SSE2       0x02
#define SIMD_SSE3       0x04
#define SIMD_SSSE3      0x08
#define SIMD_SSE4_1     0x10
#define SIMD_SSE4_2     0x20
#define SIMD_AVX        0x40
#define SIMD_AVX2       0x80
#define SIMD_AVX512F    0x100
#define SIMD_FMA        0x200

/* SIMD optimization types */
#define SIMD_OPT_MEMCPY     0
#define SIMD_OPT_MEMSET     1
#define SIMD_OPT_STRCMP     2
#define SIMD_OPT_CHECKSUM   3

typedef struct {
    u32 supported_sets;
    u8 sse_enabled;
    u8 avx_enabled;
    u8 avx512_enabled;
    u8 fma_enabled;
    u8 optimization_enabled;
    u32 vector_width;
    u32 cache_line_size;
} simd_info_t;

static simd_info_t simd_info;

extern u64 msr_read(u32 msr);
extern void msr_write(u32 msr, u64 value);
extern u8 msr_is_supported(void);

static void simd_detect_support(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check basic SIMD support */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    
    if (edx & (1 << 25)) simd_info.supported_sets |= SIMD_SSE;
    if (edx & (1 << 26)) simd_info.supported_sets |= SIMD_SSE2;
    if (ecx & (1 << 0))  simd_info.supported_sets |= SIMD_SSE3;
    if (ecx & (1 << 9))  simd_info.supported_sets |= SIMD_SSSE3;
    if (ecx & (1 << 19)) simd_info.supported_sets |= SIMD_SSE4_1;
    if (ecx & (1 << 20)) simd_info.supported_sets |= SIMD_SSE4_2;
    if (ecx & (1 << 28)) simd_info.supported_sets |= SIMD_AVX;
    if (ecx & (1 << 12)) simd_info.supported_sets |= SIMD_FMA;
    
    /* Check extended SIMD support */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    
    if (ebx & (1 << 5))  simd_info.supported_sets |= SIMD_AVX2;
    if (ebx & (1 << 16)) simd_info.supported_sets |= SIMD_AVX512F;
    
    /* Determine vector width */
    if (simd_info.supported_sets & SIMD_AVX512F) {
        simd_info.vector_width = 64; /* 512-bit */
    } else if (simd_info.supported_sets & SIMD_AVX) {
        simd_info.vector_width = 32; /* 256-bit */
    } else if (simd_info.supported_sets & SIMD_SSE) {
        simd_info.vector_width = 16; /* 128-bit */
    } else {
        simd_info.vector_width = 8;  /* 64-bit MMX fallback */
    }
}

static void simd_enable_features(void) {
    u32 cr0, cr4;
    
    /* Enable SSE if supported */
    if (simd_info.supported_sets & SIMD_SSE) {
        __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
        cr0 &= ~(1 << 2);  /* Clear EM bit */
        cr0 |= (1 << 1);   /* Set MP bit */
        __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
        
        __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
        cr4 |= (1 << 9);   /* Set OSFXSR bit */
        cr4 |= (1 << 10);  /* Set OSXMMEXCPT bit */
        __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));
        
        simd_info.sse_enabled = 1;
    }
    
    /* Enable AVX if supported */
    if (simd_info.supported_sets & SIMD_AVX) {
        extern u8 xsave_is_supported(void);
        extern u8 xsave_enable_feature(u32 feature_bit);
        
        if (xsave_is_supported()) {
            if (xsave_enable_feature(2)) { /* AVX state */
                simd_info.avx_enabled = 1;
            }
        }
    }
    
    /* Enable AVX-512 if supported */
    if (simd_info.supported_sets & SIMD_AVX512F && simd_info.avx_enabled) {
        extern u8 xsave_enable_feature(u32 feature_bit);
        
        if (xsave_enable_feature(5) && /* AVX-512 opmask */
            xsave_enable_feature(6) && /* AVX-512 ZMM_Hi256 */
            xsave_enable_feature(7)) { /* AVX-512 Hi16_ZMM */
            simd_info.avx512_enabled = 1;
        }
    }
    
    /* Enable FMA if supported */
    if (simd_info.supported_sets & SIMD_FMA && simd_info.avx_enabled) {
        simd_info.fma_enabled = 1;
    }
}

void simd_init(void) {
    simd_info.supported_sets = 0;
    simd_info.sse_enabled = 0;
    simd_info.avx_enabled = 0;
    simd_info.avx512_enabled = 0;
    simd_info.fma_enabled = 0;
    simd_info.optimization_enabled = 0;
    simd_info.vector_width = 8;
    simd_info.cache_line_size = 64;
    
    simd_detect_support();
    simd_enable_features();
    
    if (simd_info.sse_enabled || simd_info.avx_enabled) {
        simd_info.optimization_enabled = 1;
    }
}

u8 simd_is_supported(u32 instruction_set) {
    return (simd_info.supported_sets & instruction_set) != 0;
}

u8 simd_is_enabled(u32 instruction_set) {
    switch (instruction_set) {
        case SIMD_SSE:
        case SIMD_SSE2:
        case SIMD_SSE3:
        case SIMD_SSSE3:
        case SIMD_SSE4_1:
        case SIMD_SSE4_2:
            return simd_info.sse_enabled;
        case SIMD_AVX:
        case SIMD_AVX2:
            return simd_info.avx_enabled;
        case SIMD_AVX512F:
            return simd_info.avx512_enabled;
        case SIMD_FMA:
            return simd_info.fma_enabled;
        default:
            return 0;
    }
}

u32 simd_get_vector_width(void) {
    return simd_info.vector_width;
}

u8 simd_is_optimization_enabled(void) {
    return simd_info.optimization_enabled;
}

/* Optimized memory copy using SIMD */
void simd_memcpy(void* dest, const void* src, u32 size) {
    if (!simd_info.optimization_enabled || size < 16) {
        /* Fallback to regular memcpy */
        u8* d = (u8*)dest;
        const u8* s = (const u8*)src;
        for (u32 i = 0; i < size; i++) {
            d[i] = s[i];
        }
        return;
    }
    
    u8* d = (u8*)dest;
    const u8* s = (const u8*)src;
    u32 remaining = size;
    
    /* Use largest available SIMD instructions */
    if (simd_info.avx512_enabled && remaining >= 64) {
        while (remaining >= 64) {
            __asm__ volatile(
                "vmovdqu64 (%1), %%zmm0\n\t"
                "vmovdqu64 %%zmm0, (%0)"
                : : "r"(d), "r"(s) : "zmm0", "memory"
            );
            d += 64;
            s += 64;
            remaining -= 64;
        }
    } else if (simd_info.avx_enabled && remaining >= 32) {
        while (remaining >= 32) {
            __asm__ volatile(
                "vmovdqu (%1), %%ymm0\n\t"
                "vmovdqu %%ymm0, (%0)"
                : : "r"(d), "r"(s) : "ymm0", "memory"
            );
            d += 32;
            s += 32;
            remaining -= 32;
        }
    } else if (simd_info.sse_enabled && remaining >= 16) {
        while (remaining >= 16) {
            __asm__ volatile(
                "movdqu (%1), %%xmm0\n\t"
                "movdqu %%xmm0, (%0)"
                : : "r"(d), "r"(s) : "xmm0", "memory"
            );
            d += 16;
            s += 16;
            remaining -= 16;
        }
    }
    
    /* Copy remaining bytes */
    for (u32 i = 0; i < remaining; i++) {
        d[i] = s[i];
    }
}

/* Optimized memory set using SIMD */
void simd_memset(void* dest, u8 value, u32 size) {
    if (!simd_info.optimization_enabled || size < 16) {
        /* Fallback to regular memset */
        u8* d = (u8*)dest;
        for (u32 i = 0; i < size; i++) {
            d[i] = value;
        }
        return;
    }
    
    u8* d = (u8*)dest;
    u32 remaining = size;
    
    /* Prepare SIMD registers with the value */
    if (simd_info.avx512_enabled && remaining >= 64) {
        __asm__ volatile(
            "vpbroadcastb %0, %%zmm0"
            : : "r"(value) : "zmm0"
        );
        
        while (remaining >= 64) {
            __asm__ volatile(
                "vmovdqu64 %%zmm0, (%0)"
                : : "r"(d) : "memory"
            );
            d += 64;
            remaining -= 64;
        }
    } else if (simd_info.avx_enabled && remaining >= 32) {
        __asm__ volatile(
            "vpbroadcastb %0, %%ymm0"
            : : "r"(value) : "ymm0"
        );
        
        while (remaining >= 32) {
            __asm__ volatile(
                "vmovdqu %%ymm0, (%0)"
                : : "r"(d) : "memory"
            );
            d += 32;
            remaining -= 32;
        }
    } else if (simd_info.sse_enabled && remaining >= 16) {
        /* Broadcast byte to XMM register */
        u32 broadcast = value | (value << 8) | (value << 16) | (value << 24);
        __asm__ volatile(
            "movd %0, %%xmm0\n\t"
            "pshufd $0, %%xmm0, %%xmm0"
            : : "r"(broadcast) : "xmm0"
        );
        
        while (remaining >= 16) {
            __asm__ volatile(
                "movdqu %%xmm0, (%0)"
                : : "r"(d) : "memory"
            );
            d += 16;
            remaining -= 16;
        }
    }
    
    /* Set remaining bytes */
    for (u32 i = 0; i < remaining; i++) {
        d[i] = value;
    }
}

/* Optimized string comparison using SIMD */
s32 simd_strcmp(const char* str1, const char* str2) {
    if (!simd_info.optimization_enabled || !simd_is_supported(SIMD_SSE4_2)) {
        /* Fallback to regular strcmp */
        const u8* s1 = (const u8*)str1;
        const u8* s2 = (const u8*)str2;
        
        while (*s1 && *s1 == *s2) {
            s1++;
            s2++;
        }
        
        return *s1 - *s2;
    }
    
    const char* s1 = str1;
    const char* s2 = str2;
    
    /* Use SSE4.2 string comparison instructions */
    while (1) {
        u32 result;
        __asm__ volatile(
            "movdqu (%1), %%xmm0\n\t"
            "movdqu (%2), %%xmm1\n\t"
            "pcmpistri $0x18, %%xmm1, %%xmm0\n\t"
            "movl %%ecx, %0"
            : "=r"(result) : "r"(s1), "r"(s2) : "xmm0", "xmm1", "ecx"
        );
        
        if (result < 16) {
            return s1[result] - s2[result];
        }
        
        /* Check for null terminator */
        u8 found_null = 0;
        for (u32 i = 0; i < 16; i++) {
            if (s1[i] == 0 || s2[i] == 0) {
                found_null = 1;
                break;
            }
        }
        
        if (found_null) break;
        
        s1 += 16;
        s2 += 16;
    }
    
    /* Fallback for remaining characters */
    while (*s1 && *s1 == *s2) {
        s1++;
        s2++;
    }
    
    return *s1 - *s2;
}

/* Optimized checksum calculation using SIMD */
u32 simd_checksum(const void* data, u32 size) {
    if (!simd_info.optimization_enabled || size < 16) {
        /* Fallback to regular checksum */
        const u8* d = (const u8*)data;
        u32 sum = 0;
        for (u32 i = 0; i < size; i++) {
            sum += d[i];
        }
        return sum;
    }
    
    const u8* d = (const u8*)data;
    u32 remaining = size;
    u32 sum = 0;
    
    if (simd_info.sse_enabled && remaining >= 16) {
        /* Initialize accumulator */
        __asm__ volatile("pxor %%xmm0, %%xmm0" : : : "xmm0");
        
        while (remaining >= 16) {
            __asm__ volatile(
                "movdqu (%0), %%xmm1\n\t"
                "psadbw %%xmm1, %%xmm0"
                : : "r"(d) : "xmm0", "xmm1"
            );
            d += 16;
            remaining -= 16;
        }
        
        /* Extract sum from XMM register */
        u32 partial_sums[4];
        __asm__ volatile(
            "movdqu %%xmm0, %0"
            : "=m"(partial_sums) : : "xmm0"
        );
        
        sum = partial_sums[0] + partial_sums[2];
    }
    
    /* Add remaining bytes */
    for (u32 i = 0; i < remaining; i++) {
        sum += d[i];
    }
    
    return sum;
}

void simd_save_state(void* state_area) {
    if (!simd_info.optimization_enabled) return;
    
    if (simd_info.avx512_enabled) {
        extern void xsave_save_state(void* xsave_area, u64 feature_mask);
        xsave_save_state(state_area, 0xE7); /* All AVX-512 features */
    } else if (simd_info.avx_enabled) {
        extern void xsave_save_state(void* xsave_area, u64 feature_mask);
        xsave_save_state(state_area, 0x7); /* x87, SSE, AVX */
    } else if (simd_info.sse_enabled) {
        __asm__ volatile("fxsave (%0)" : : "r"(state_area) : "memory");
    }
}

void simd_restore_state(const void* state_area) {
    if (!simd_info.optimization_enabled) return;
    
    if (simd_info.avx512_enabled) {
        extern void xsave_restore_state(const void* xsave_area, u64 feature_mask);
        xsave_restore_state(state_area, 0xE7); /* All AVX-512 features */
    } else if (simd_info.avx_enabled) {
        extern void xsave_restore_state(const void* xsave_area, u64 feature_mask);
        xsave_restore_state(state_area, 0x7); /* x87, SSE, AVX */
    } else if (simd_info.sse_enabled) {
        __asm__ volatile("fxrstor (%0)" : : "r"(state_area) : "memory");
    }
}

u32 simd_get_state_size(void) {
    if (simd_info.avx512_enabled || simd_info.avx_enabled) {
        extern u32 xsave_get_area_size(void);
        return xsave_get_area_size();
    } else if (simd_info.sse_enabled) {
        return 512; /* FXSAVE area size */
    }
    return 0;
}
