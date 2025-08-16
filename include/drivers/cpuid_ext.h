/*
 * cpuid_ext.h – x86 extended CPUID feature detection and management
 */

#ifndef CPUID_EXT_H
#define CPUID_EXT_H

#include "kernel/types.h"

/* Feature flags structure */
typedef struct {
    /* Leaf 1 ECX features */
    u8 sse3;
    u8 pclmulqdq;
    u8 dtes64;
    u8 monitor;
    u8 ds_cpl;
    u8 vmx;
    u8 smx;
    u8 eist;
    u8 tm2;
    u8 ssse3;
    u8 cnxt_id;
    u8 sdbg;
    u8 fma;
    u8 cmpxchg16b;
    u8 xtpr;
    u8 pdcm;
    u8 pcid;
    u8 dca;
    u8 sse4_1;
    u8 sse4_2;
    u8 x2apic;
    u8 movbe;
    u8 popcnt;
    u8 tsc_deadline;
    u8 aes;
    u8 xsave;
    u8 osxsave;
    u8 avx;
    u8 f16c;
    u8 rdrand;
    u8 hypervisor;
    
    /* Leaf 1 EDX features */
    u8 fpu;
    u8 vme;
    u8 de;
    u8 pse;
    u8 tsc;
    u8 msr;
    u8 pae;
    u8 mce;
    u8 cx8;
    u8 apic;
    u8 sep;
    u8 mtrr;
    u8 pge;
    u8 mca;
    u8 cmov;
    u8 pat;
    u8 pse36;
    u8 psn;
    u8 clfsh;
    u8 ds;
    u8 acpi;
    u8 mmx;
    u8 fxsr;
    u8 sse;
    u8 sse2;
    u8 ss;
    u8 htt;
    u8 tm;
    u8 ia64;
    u8 pbe;
    
    /* Leaf 7 EBX features */
    u8 fsgsbase;
    u8 tsc_adjust;
    u8 sgx;
    u8 bmi1;
    u8 hle;
    u8 avx2;
    u8 fdp_excptn_only;
    u8 smep;
    u8 bmi2;
    u8 erms;
    u8 invpcid;
    u8 rtm;
    u8 pqm;
    u8 fpcsds;
    u8 mpx;
    u8 pqe;
    u8 avx512f;
    u8 avx512dq;
    u8 rdseed;
    u8 adx;
    u8 smap;
    u8 avx512ifma;
    u8 pcommit;
    u8 clflushopt;
    u8 clwb;
    u8 intel_pt;
    u8 avx512pf;
    u8 avx512er;
    u8 avx512cd;
    u8 sha;
    u8 avx512bw;
    u8 avx512vl;
    
    /* Additional features */
    u8 prefetchwt1;
    u8 avx512vbmi;
    u8 umip;
    u8 pku;
    u8 ospke;
    u8 waitpkg;
    u8 avx512vbmi2;
    u8 cet_ss;
    u8 gfni;
    u8 vaes;
    u8 vpclmulqdq;
    u8 avx512vnni;
    u8 avx512bitalg;
    u8 tme;
    u8 avx512vpopcntdq;
    u8 la57;
    u8 rdpid;
    u8 kl;
    u8 bus_lock_detect;
    u8 cldemote;
    u8 movdiri;
    u8 movdir64b;
    u8 enqcmd;
    u8 sgx_lc;
    u8 pks;
    u8 avx512_4vnniw;
    u8 avx512_4fmaps;
    u8 fsrm;
    u8 uintr;
    u8 avx512_vp2intersect;
    u8 srbds_ctrl;
    u8 md_clear;
    u8 rtm_always_abort;
    u8 tsx_force_abort;
    u8 serialize;
    u8 hybrid;
    u8 tsxldtrk;
    u8 pconfig;
    u8 lbr;
    u8 cet_ibt;
    u8 amx_bf16;
    u8 avx512_fp16;
    u8 amx_tile;
    u8 amx_int8;
    u8 ibrs_ibpb;
    u8 stibp;
    u8 l1d_flush;
    u8 ia32_arch_capabilities;
    u8 ia32_core_capabilities;
    u8 ssbd;
} cpuid_features_t;

/* Core functions */
void cpuid_ext_init(void);

/* Feature detection */
u8 cpuid_ext_is_feature_supported(const char* feature_name);
const cpuid_features_t* cpuid_ext_get_features(void);

/* CPU information */
const char* cpuid_ext_get_vendor_string(void);
const char* cpuid_ext_get_brand_string(void);
u32 cpuid_ext_get_signature(void);
u8 cpuid_ext_get_family(void);
u8 cpuid_ext_get_model(void);
u8 cpuid_ext_get_stepping(void);
u8 cpuid_ext_get_brand_id(void);
u8 cpuid_ext_get_cache_line_size(void);
u8 cpuid_ext_get_max_logical_processors(void);
u8 cpuid_ext_get_initial_apic_id(void);

/* CPUID leaf information */
u32 cpuid_ext_get_max_basic_leaf(void);
u32 cpuid_ext_get_max_extended_leaf(void);

#endif
