/*
 * cpuid.h – x86 CPU identification
 */

#ifndef CPUID_H
#define CPUID_H

#include "kernel/types.h"

/* Feature constants */
#define CPU_FEATURE_FPU       0
#define CPU_FEATURE_TSC       1
#define CPU_FEATURE_MSR       2
#define CPU_FEATURE_APIC      3
#define CPU_FEATURE_MTRR      4
#define CPU_FEATURE_SSE       5
#define CPU_FEATURE_SSE2      6
#define CPU_FEATURE_SSE3      7
#define CPU_FEATURE_SSSE3     8
#define CPU_FEATURE_SSE41     9
#define CPU_FEATURE_SSE42     10
#define CPU_FEATURE_AVX       11
#define CPU_FEATURE_AVX2      12
#define CPU_FEATURE_AES       13
#define CPU_FEATURE_RDRAND    14
#define CPU_FEATURE_HYPERVISOR 15

/* Core functions */
void cpuid_init(void);

/* CPU information */
const char* cpuid_get_vendor(void);
const char* cpuid_get_brand(void);
u32 cpuid_get_family(void);
u32 cpuid_get_model(void);
u32 cpuid_get_stepping(void);

/* Feature detection */
u8 cpuid_has_feature(u32 feature);
const char* cpuid_get_feature_name(u32 feature);
u32 cpuid_get_features_edx(void);
u32 cpuid_get_features_ecx(void);

/* Vendor detection */
u8 cpuid_is_intel(void);
u8 cpuid_is_amd(void);

/* Special instructions */
u64 cpuid_rdtsc(void);
u32 cpuid_rdrand(void);

#endif
