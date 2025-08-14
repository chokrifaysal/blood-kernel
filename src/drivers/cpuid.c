/*
 * cpuid.c – x86 CPU identification and feature detection
 */

#include "kernel/types.h"

typedef struct {
    char vendor[13];
    char brand[49];
    u32 family;
    u32 model;
    u32 stepping;
    u32 features_edx;
    u32 features_ecx;
    u32 extended_features;
    u32 cache_info[16];
    u8 has_fpu;
    u8 has_tsc;
    u8 has_msr;
    u8 has_apic;
    u8 has_mtrr;
    u8 has_sse;
    u8 has_sse2;
    u8 has_sse3;
    u8 has_ssse3;
    u8 has_sse41;
    u8 has_sse42;
    u8 has_avx;
    u8 has_avx2;
    u8 has_aes;
    u8 has_rdrand;
    u8 has_hypervisor;
} cpu_info_t;

static cpu_info_t cpu_info;

extern u8 cpuid_supported(void);
extern void get_cpuid(u32 function, u32* output);

static void cpuid(u32 function, u32* eax, u32* ebx, u32* ecx, u32* edx) {
    u32 output[4];
    get_cpuid(function, output);
    *eax = output[0];
    *ebx = output[1];
    *ecx = output[2];
    *edx = output[3];
}

void cpuid_init(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Clear CPU info */
    for (u8 i = 0; i < sizeof(cpu_info); i++) {
        ((u8*)&cpu_info)[i] = 0;
    }
    
    if (!cpuid_supported()) {
        return; /* CPUID not supported */
    }
    
    /* Get vendor string */
    cpuid(0, &eax, &ebx, &ecx, &edx);
    *(u32*)&cpu_info.vendor[0] = ebx;
    *(u32*)&cpu_info.vendor[4] = edx;
    *(u32*)&cpu_info.vendor[8] = ecx;
    cpu_info.vendor[12] = 0;
    
    /* Get basic CPU info */
    if (eax >= 1) {
        cpuid(1, &eax, &ebx, &ecx, &edx);
        
        cpu_info.stepping = eax & 0xF;
        cpu_info.model = (eax >> 4) & 0xF;
        cpu_info.family = (eax >> 8) & 0xF;
        
        /* Extended model and family */
        if (cpu_info.family == 0xF) {
            cpu_info.family += (eax >> 20) & 0xFF;
        }
        if (cpu_info.family == 0x6 || cpu_info.family == 0xF) {
            cpu_info.model += ((eax >> 16) & 0xF) << 4;
        }
        
        cpu_info.features_edx = edx;
        cpu_info.features_ecx = ecx;
        
        /* Feature flags */
        cpu_info.has_fpu = (edx & (1 << 0)) != 0;
        cpu_info.has_tsc = (edx & (1 << 4)) != 0;
        cpu_info.has_msr = (edx & (1 << 5)) != 0;
        cpu_info.has_apic = (edx & (1 << 9)) != 0;
        cpu_info.has_mtrr = (edx & (1 << 12)) != 0;
        cpu_info.has_sse = (edx & (1 << 25)) != 0;
        cpu_info.has_sse2 = (edx & (1 << 26)) != 0;
        cpu_info.has_sse3 = (ecx & (1 << 0)) != 0;
        cpu_info.has_ssse3 = (ecx & (1 << 9)) != 0;
        cpu_info.has_sse41 = (ecx & (1 << 19)) != 0;
        cpu_info.has_sse42 = (ecx & (1 << 20)) != 0;
        cpu_info.has_aes = (ecx & (1 << 25)) != 0;
        cpu_info.has_avx = (ecx & (1 << 28)) != 0;
        cpu_info.has_rdrand = (ecx & (1 << 30)) != 0;
        cpu_info.has_hypervisor = (ecx & (1 << 31)) != 0;
    }
    
    /* Get extended features */
    cpuid(0x80000000, &eax, &ebx, &ecx, &edx);
    if (eax >= 0x80000001) {
        cpuid(0x80000001, &eax, &ebx, &ecx, &edx);
        cpu_info.extended_features = edx;
    }
    
    /* Get brand string */
    if (eax >= 0x80000004) {
        cpuid(0x80000002, &eax, &ebx, &ecx, &edx);
        *(u32*)&cpu_info.brand[0] = eax;
        *(u32*)&cpu_info.brand[4] = ebx;
        *(u32*)&cpu_info.brand[8] = ecx;
        *(u32*)&cpu_info.brand[12] = edx;
        
        cpuid(0x80000003, &eax, &ebx, &ecx, &edx);
        *(u32*)&cpu_info.brand[16] = eax;
        *(u32*)&cpu_info.brand[20] = ebx;
        *(u32*)&cpu_info.brand[24] = ecx;
        *(u32*)&cpu_info.brand[28] = edx;
        
        cpuid(0x80000004, &eax, &ebx, &ecx, &edx);
        *(u32*)&cpu_info.brand[32] = eax;
        *(u32*)&cpu_info.brand[36] = ebx;
        *(u32*)&cpu_info.brand[40] = ecx;
        *(u32*)&cpu_info.brand[44] = edx;
        
        cpu_info.brand[48] = 0;
        
        /* Trim leading spaces */
        char* brand = cpu_info.brand;
        while (*brand == ' ') brand++;
        if (brand != cpu_info.brand) {
            for (u8 i = 0; i < 48; i++) {
                cpu_info.brand[i] = brand[i];
                if (!brand[i]) break;
            }
        }
    }
    
    /* Get cache information */
    cpuid(0, &eax, &ebx, &ecx, &edx);
    if (eax >= 2) {
        cpuid(2, &eax, &ebx, &ecx, &edx);
        cpu_info.cache_info[0] = eax;
        cpu_info.cache_info[1] = ebx;
        cpu_info.cache_info[2] = ecx;
        cpu_info.cache_info[3] = edx;
    }
    
    /* Check for AVX2 */
    cpuid(0, &eax, &ebx, &ecx, &edx);
    if (eax >= 7) {
        cpuid(7, &eax, &ebx, &ecx, &edx);
        cpu_info.has_avx2 = (ebx & (1 << 5)) != 0;
    }
}

const char* cpuid_get_vendor(void) {
    return cpu_info.vendor;
}

const char* cpuid_get_brand(void) {
    return cpu_info.brand[0] ? cpu_info.brand : "Unknown CPU";
}

u32 cpuid_get_family(void) {
    return cpu_info.family;
}

u32 cpuid_get_model(void) {
    return cpu_info.model;
}

u32 cpuid_get_stepping(void) {
    return cpu_info.stepping;
}

u8 cpuid_has_feature(u32 feature) {
    switch (feature) {
        case 0: return cpu_info.has_fpu;
        case 1: return cpu_info.has_tsc;
        case 2: return cpu_info.has_msr;
        case 3: return cpu_info.has_apic;
        case 4: return cpu_info.has_mtrr;
        case 5: return cpu_info.has_sse;
        case 6: return cpu_info.has_sse2;
        case 7: return cpu_info.has_sse3;
        case 8: return cpu_info.has_ssse3;
        case 9: return cpu_info.has_sse41;
        case 10: return cpu_info.has_sse42;
        case 11: return cpu_info.has_avx;
        case 12: return cpu_info.has_avx2;
        case 13: return cpu_info.has_aes;
        case 14: return cpu_info.has_rdrand;
        case 15: return cpu_info.has_hypervisor;
        default: return 0;
    }
}

const char* cpuid_get_feature_name(u32 feature) {
    static const char* feature_names[] = {
        "FPU", "TSC", "MSR", "APIC", "MTRR",
        "SSE", "SSE2", "SSE3", "SSSE3", "SSE4.1",
        "SSE4.2", "AVX", "AVX2", "AES", "RDRAND", "Hypervisor"
    };
    
    if (feature < 16) {
        return feature_names[feature];
    }
    return "Unknown";
}

u32 cpuid_get_features_edx(void) {
    return cpu_info.features_edx;
}

u32 cpuid_get_features_ecx(void) {
    return cpu_info.features_ecx;
}

u8 cpuid_is_intel(void) {
    return (cpu_info.vendor[0] == 'G' && 
            cpu_info.vendor[1] == 'e' && 
            cpu_info.vendor[2] == 'n');
}

u8 cpuid_is_amd(void) {
    return (cpu_info.vendor[0] == 'A' && 
            cpu_info.vendor[1] == 'u' && 
            cpu_info.vendor[2] == 't');
}

u64 cpuid_rdtsc(void) {
    if (!cpu_info.has_tsc) return 0;
    
    u32 low, high;
    __asm__ volatile("rdtsc" : "=a"(low), "=d"(high));
    return ((u64)high << 32) | low;
}

u32 cpuid_rdrand(void) {
    if (!cpu_info.has_rdrand) return 0;
    
    u32 value;
    u8 success;
    __asm__ volatile("rdrand %0; setc %1" : "=r"(value), "=qm"(success));
    return success ? value : 0;
}
