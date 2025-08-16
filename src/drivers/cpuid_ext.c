/*
 * cpuid_ext.c – x86 extended CPUID feature detection and management
 */

#include "kernel/types.h"

/* Extended CPUID leaves */
#define CPUID_EXTENDED_FEATURES     0x7
#define CPUID_EXTENDED_STATE        0xD
#define CPUID_PROCESSOR_FREQUENCY   0x16
#define CPUID_SOC_VENDOR            0x17
#define CPUID_DETERMINISTIC_CACHE   0x4
#define CPUID_MONITOR_MWAIT         0x5
#define CPUID_THERMAL_POWER         0x6
#define CPUID_STRUCTURED_EXTENDED   0x7
#define CPUID_DIRECT_CACHE_ACCESS   0x9
#define CPUID_ARCHITECTURAL_PERFMON 0xA
#define CPUID_EXTENDED_TOPOLOGY     0xB
#define CPUID_PROCESSOR_TRACE       0x14
#define CPUID_TIME_STAMP_COUNTER    0x15

/* Feature flags from various CPUID leaves */
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
    
    /* Leaf 7 ECX features */
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
    
    /* Leaf 7 EDX features */
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

typedef struct {
    u8 features_detected;
    cpuid_features_t features;
    u32 max_basic_leaf;
    u32 max_extended_leaf;
    u32 max_subleaf_7;
    char vendor_string[13];
    char brand_string[49];
    u32 signature;
    u8 family;
    u8 model;
    u8 stepping;
    u8 brand_id;
    u8 cache_line_size;
    u8 max_logical_processors;
    u8 initial_apic_id;
} cpuid_ext_info_t;

static cpuid_ext_info_t cpuid_ext_info;

static void cpuid_ext_get_vendor_string(void) {
    u32 eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0));
    
    *(u32*)&cpuid_ext_info.vendor_string[0] = ebx;
    *(u32*)&cpuid_ext_info.vendor_string[4] = edx;
    *(u32*)&cpuid_ext_info.vendor_string[8] = ecx;
    cpuid_ext_info.vendor_string[12] = '\0';
    
    cpuid_ext_info.max_basic_leaf = eax;
}

static void cpuid_ext_get_brand_string(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check if brand string is supported */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x80000000));
    if (eax < 0x80000004) {
        cpuid_ext_info.brand_string[0] = '\0';
        return;
    }
    
    /* Get brand string parts */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x80000002));
    *(u32*)&cpuid_ext_info.brand_string[0] = eax;
    *(u32*)&cpuid_ext_info.brand_string[4] = ebx;
    *(u32*)&cpuid_ext_info.brand_string[8] = ecx;
    *(u32*)&cpuid_ext_info.brand_string[12] = edx;
    
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x80000003));
    *(u32*)&cpuid_ext_info.brand_string[16] = eax;
    *(u32*)&cpuid_ext_info.brand_string[20] = ebx;
    *(u32*)&cpuid_ext_info.brand_string[24] = ecx;
    *(u32*)&cpuid_ext_info.brand_string[28] = edx;
    
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x80000004));
    *(u32*)&cpuid_ext_info.brand_string[32] = eax;
    *(u32*)&cpuid_ext_info.brand_string[36] = ebx;
    *(u32*)&cpuid_ext_info.brand_string[40] = ecx;
    *(u32*)&cpuid_ext_info.brand_string[44] = edx;
    
    cpuid_ext_info.brand_string[48] = '\0';
}

static void cpuid_ext_detect_basic_features(void) {
    u32 eax, ebx, ecx, edx;
    
    if (cpuid_ext_info.max_basic_leaf < 1) return;
    
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    
    cpuid_ext_info.signature = eax;
    cpuid_ext_info.stepping = eax & 0xF;
    cpuid_ext_info.model = (eax >> 4) & 0xF;
    cpuid_ext_info.family = (eax >> 8) & 0xF;
    
    /* Handle extended family/model */
    if (cpuid_ext_info.family == 0xF) {
        cpuid_ext_info.family += (eax >> 20) & 0xFF;
    }
    if (cpuid_ext_info.family == 0x6 || cpuid_ext_info.family == 0xF) {
        cpuid_ext_info.model += ((eax >> 16) & 0xF) << 4;
    }
    
    cpuid_ext_info.brand_id = ebx & 0xFF;
    cpuid_ext_info.cache_line_size = ((ebx >> 8) & 0xFF) * 8;
    cpuid_ext_info.max_logical_processors = (ebx >> 16) & 0xFF;
    cpuid_ext_info.initial_apic_id = (ebx >> 24) & 0xFF;
    
    /* ECX features */
    cpuid_ext_info.features.sse3 = (ecx & (1 << 0)) != 0;
    cpuid_ext_info.features.pclmulqdq = (ecx & (1 << 1)) != 0;
    cpuid_ext_info.features.dtes64 = (ecx & (1 << 2)) != 0;
    cpuid_ext_info.features.monitor = (ecx & (1 << 3)) != 0;
    cpuid_ext_info.features.ds_cpl = (ecx & (1 << 4)) != 0;
    cpuid_ext_info.features.vmx = (ecx & (1 << 5)) != 0;
    cpuid_ext_info.features.smx = (ecx & (1 << 6)) != 0;
    cpuid_ext_info.features.eist = (ecx & (1 << 7)) != 0;
    cpuid_ext_info.features.tm2 = (ecx & (1 << 8)) != 0;
    cpuid_ext_info.features.ssse3 = (ecx & (1 << 9)) != 0;
    cpuid_ext_info.features.cnxt_id = (ecx & (1 << 10)) != 0;
    cpuid_ext_info.features.sdbg = (ecx & (1 << 11)) != 0;
    cpuid_ext_info.features.fma = (ecx & (1 << 12)) != 0;
    cpuid_ext_info.features.cmpxchg16b = (ecx & (1 << 13)) != 0;
    cpuid_ext_info.features.xtpr = (ecx & (1 << 14)) != 0;
    cpuid_ext_info.features.pdcm = (ecx & (1 << 15)) != 0;
    cpuid_ext_info.features.pcid = (ecx & (1 << 17)) != 0;
    cpuid_ext_info.features.dca = (ecx & (1 << 18)) != 0;
    cpuid_ext_info.features.sse4_1 = (ecx & (1 << 19)) != 0;
    cpuid_ext_info.features.sse4_2 = (ecx & (1 << 20)) != 0;
    cpuid_ext_info.features.x2apic = (ecx & (1 << 21)) != 0;
    cpuid_ext_info.features.movbe = (ecx & (1 << 22)) != 0;
    cpuid_ext_info.features.popcnt = (ecx & (1 << 23)) != 0;
    cpuid_ext_info.features.tsc_deadline = (ecx & (1 << 24)) != 0;
    cpuid_ext_info.features.aes = (ecx & (1 << 25)) != 0;
    cpuid_ext_info.features.xsave = (ecx & (1 << 26)) != 0;
    cpuid_ext_info.features.osxsave = (ecx & (1 << 27)) != 0;
    cpuid_ext_info.features.avx = (ecx & (1 << 28)) != 0;
    cpuid_ext_info.features.f16c = (ecx & (1 << 29)) != 0;
    cpuid_ext_info.features.rdrand = (ecx & (1 << 30)) != 0;
    cpuid_ext_info.features.hypervisor = (ecx & (1 << 31)) != 0;
    
    /* EDX features */
    cpuid_ext_info.features.fpu = (edx & (1 << 0)) != 0;
    cpuid_ext_info.features.vme = (edx & (1 << 1)) != 0;
    cpuid_ext_info.features.de = (edx & (1 << 2)) != 0;
    cpuid_ext_info.features.pse = (edx & (1 << 3)) != 0;
    cpuid_ext_info.features.tsc = (edx & (1 << 4)) != 0;
    cpuid_ext_info.features.msr = (edx & (1 << 5)) != 0;
    cpuid_ext_info.features.pae = (edx & (1 << 6)) != 0;
    cpuid_ext_info.features.mce = (edx & (1 << 7)) != 0;
    cpuid_ext_info.features.cx8 = (edx & (1 << 8)) != 0;
    cpuid_ext_info.features.apic = (edx & (1 << 9)) != 0;
    cpuid_ext_info.features.sep = (edx & (1 << 11)) != 0;
    cpuid_ext_info.features.mtrr = (edx & (1 << 12)) != 0;
    cpuid_ext_info.features.pge = (edx & (1 << 13)) != 0;
    cpuid_ext_info.features.mca = (edx & (1 << 14)) != 0;
    cpuid_ext_info.features.cmov = (edx & (1 << 15)) != 0;
    cpuid_ext_info.features.pat = (edx & (1 << 16)) != 0;
    cpuid_ext_info.features.pse36 = (edx & (1 << 17)) != 0;
    cpuid_ext_info.features.psn = (edx & (1 << 18)) != 0;
    cpuid_ext_info.features.clfsh = (edx & (1 << 19)) != 0;
    cpuid_ext_info.features.ds = (edx & (1 << 21)) != 0;
    cpuid_ext_info.features.acpi = (edx & (1 << 22)) != 0;
    cpuid_ext_info.features.mmx = (edx & (1 << 23)) != 0;
    cpuid_ext_info.features.fxsr = (edx & (1 << 24)) != 0;
    cpuid_ext_info.features.sse = (edx & (1 << 25)) != 0;
    cpuid_ext_info.features.sse2 = (edx & (1 << 26)) != 0;
    cpuid_ext_info.features.ss = (edx & (1 << 27)) != 0;
    cpuid_ext_info.features.htt = (edx & (1 << 28)) != 0;
    cpuid_ext_info.features.tm = (edx & (1 << 29)) != 0;
    cpuid_ext_info.features.ia64 = (edx & (1 << 30)) != 0;
    cpuid_ext_info.features.pbe = (edx & (1 << 31)) != 0;
}

static void cpuid_ext_detect_extended_features(void) {
    u32 eax, ebx, ecx, edx;
    
    if (cpuid_ext_info.max_basic_leaf < 7) return;
    
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    
    cpuid_ext_info.max_subleaf_7 = eax;
    
    /* EBX features */
    cpuid_ext_info.features.fsgsbase = (ebx & (1 << 0)) != 0;
    cpuid_ext_info.features.tsc_adjust = (ebx & (1 << 1)) != 0;
    cpuid_ext_info.features.sgx = (ebx & (1 << 2)) != 0;
    cpuid_ext_info.features.bmi1 = (ebx & (1 << 3)) != 0;
    cpuid_ext_info.features.hle = (ebx & (1 << 4)) != 0;
    cpuid_ext_info.features.avx2 = (ebx & (1 << 5)) != 0;
    cpuid_ext_info.features.fdp_excptn_only = (ebx & (1 << 6)) != 0;
    cpuid_ext_info.features.smep = (ebx & (1 << 7)) != 0;
    cpuid_ext_info.features.bmi2 = (ebx & (1 << 8)) != 0;
    cpuid_ext_info.features.erms = (ebx & (1 << 9)) != 0;
    cpuid_ext_info.features.invpcid = (ebx & (1 << 10)) != 0;
    cpuid_ext_info.features.rtm = (ebx & (1 << 11)) != 0;
    cpuid_ext_info.features.pqm = (ebx & (1 << 12)) != 0;
    cpuid_ext_info.features.fpcsds = (ebx & (1 << 13)) != 0;
    cpuid_ext_info.features.mpx = (ebx & (1 << 14)) != 0;
    cpuid_ext_info.features.pqe = (ebx & (1 << 15)) != 0;
    cpuid_ext_info.features.avx512f = (ebx & (1 << 16)) != 0;
    cpuid_ext_info.features.avx512dq = (ebx & (1 << 17)) != 0;
    cpuid_ext_info.features.rdseed = (ebx & (1 << 18)) != 0;
    cpuid_ext_info.features.adx = (ebx & (1 << 19)) != 0;
    cpuid_ext_info.features.smap = (ebx & (1 << 20)) != 0;
    cpuid_ext_info.features.avx512ifma = (ebx & (1 << 21)) != 0;
    cpuid_ext_info.features.pcommit = (ebx & (1 << 22)) != 0;
    cpuid_ext_info.features.clflushopt = (ebx & (1 << 23)) != 0;
    cpuid_ext_info.features.clwb = (ebx & (1 << 24)) != 0;
    cpuid_ext_info.features.intel_pt = (ebx & (1 << 25)) != 0;
    cpuid_ext_info.features.avx512pf = (ebx & (1 << 26)) != 0;
    cpuid_ext_info.features.avx512er = (ebx & (1 << 27)) != 0;
    cpuid_ext_info.features.avx512cd = (ebx & (1 << 28)) != 0;
    cpuid_ext_info.features.sha = (ebx & (1 << 29)) != 0;
    cpuid_ext_info.features.avx512bw = (ebx & (1 << 30)) != 0;
    cpuid_ext_info.features.avx512vl = (ebx & (1 << 31)) != 0;
    
    /* ECX features */
    cpuid_ext_info.features.prefetchwt1 = (ecx & (1 << 0)) != 0;
    cpuid_ext_info.features.avx512vbmi = (ecx & (1 << 1)) != 0;
    cpuid_ext_info.features.umip = (ecx & (1 << 2)) != 0;
    cpuid_ext_info.features.pku = (ecx & (1 << 3)) != 0;
    cpuid_ext_info.features.ospke = (ecx & (1 << 4)) != 0;
    cpuid_ext_info.features.waitpkg = (ecx & (1 << 5)) != 0;
    cpuid_ext_info.features.avx512vbmi2 = (ecx & (1 << 6)) != 0;
    cpuid_ext_info.features.cet_ss = (ecx & (1 << 7)) != 0;
    cpuid_ext_info.features.gfni = (ecx & (1 << 8)) != 0;
    cpuid_ext_info.features.vaes = (ecx & (1 << 9)) != 0;
    cpuid_ext_info.features.vpclmulqdq = (ecx & (1 << 10)) != 0;
    cpuid_ext_info.features.avx512vnni = (ecx & (1 << 11)) != 0;
    cpuid_ext_info.features.avx512bitalg = (ecx & (1 << 12)) != 0;
    cpuid_ext_info.features.tme = (ecx & (1 << 13)) != 0;
    cpuid_ext_info.features.avx512vpopcntdq = (ecx & (1 << 14)) != 0;
    cpuid_ext_info.features.la57 = (ecx & (1 << 16)) != 0;
    cpuid_ext_info.features.rdpid = (ecx & (1 << 22)) != 0;
    cpuid_ext_info.features.kl = (ecx & (1 << 23)) != 0;
    cpuid_ext_info.features.bus_lock_detect = (ecx & (1 << 24)) != 0;
    cpuid_ext_info.features.cldemote = (ecx & (1 << 25)) != 0;
    cpuid_ext_info.features.movdiri = (ecx & (1 << 27)) != 0;
    cpuid_ext_info.features.movdir64b = (ecx & (1 << 28)) != 0;
    cpuid_ext_info.features.enqcmd = (ecx & (1 << 29)) != 0;
    cpuid_ext_info.features.sgx_lc = (ecx & (1 << 30)) != 0;
    cpuid_ext_info.features.pks = (ecx & (1 << 31)) != 0;
    
    /* EDX features */
    cpuid_ext_info.features.avx512_4vnniw = (edx & (1 << 2)) != 0;
    cpuid_ext_info.features.avx512_4fmaps = (edx & (1 << 3)) != 0;
    cpuid_ext_info.features.fsrm = (edx & (1 << 4)) != 0;
    cpuid_ext_info.features.uintr = (edx & (1 << 5)) != 0;
    cpuid_ext_info.features.avx512_vp2intersect = (edx & (1 << 8)) != 0;
    cpuid_ext_info.features.srbds_ctrl = (edx & (1 << 9)) != 0;
    cpuid_ext_info.features.md_clear = (edx & (1 << 10)) != 0;
    cpuid_ext_info.features.rtm_always_abort = (edx & (1 << 11)) != 0;
    cpuid_ext_info.features.tsx_force_abort = (edx & (1 << 13)) != 0;
    cpuid_ext_info.features.serialize = (edx & (1 << 14)) != 0;
    cpuid_ext_info.features.hybrid = (edx & (1 << 15)) != 0;
    cpuid_ext_info.features.tsxldtrk = (edx & (1 << 16)) != 0;
    cpuid_ext_info.features.pconfig = (edx & (1 << 18)) != 0;
    cpuid_ext_info.features.lbr = (edx & (1 << 19)) != 0;
    cpuid_ext_info.features.cet_ibt = (edx & (1 << 20)) != 0;
    cpuid_ext_info.features.amx_bf16 = (edx & (1 << 22)) != 0;
    cpuid_ext_info.features.avx512_fp16 = (edx & (1 << 23)) != 0;
    cpuid_ext_info.features.amx_tile = (edx & (1 << 24)) != 0;
    cpuid_ext_info.features.amx_int8 = (edx & (1 << 25)) != 0;
    cpuid_ext_info.features.ibrs_ibpb = (edx & (1 << 26)) != 0;
    cpuid_ext_info.features.stibp = (edx & (1 << 27)) != 0;
    cpuid_ext_info.features.l1d_flush = (edx & (1 << 28)) != 0;
    cpuid_ext_info.features.ia32_arch_capabilities = (edx & (1 << 29)) != 0;
    cpuid_ext_info.features.ia32_core_capabilities = (edx & (1 << 30)) != 0;
    cpuid_ext_info.features.ssbd = (edx & (1 << 31)) != 0;
}

void cpuid_ext_init(void) {
    cpuid_ext_info.features_detected = 0;
    
    cpuid_ext_get_vendor_string();
    cpuid_ext_get_brand_string();
    cpuid_ext_detect_basic_features();
    cpuid_ext_detect_extended_features();
    
    cpuid_ext_info.features_detected = 1;
}

u8 cpuid_ext_is_feature_supported(const char* feature_name) {
    if (!cpuid_ext_info.features_detected || !feature_name) return 0;
    
    /* Simple string matching for common features */
    if (feature_name[0] == 's' && feature_name[1] == 's' && feature_name[2] == 'e') {
        if (feature_name[3] == '3') return cpuid_ext_info.features.sse3;
        if (feature_name[3] == '4' && feature_name[4] == '_' && feature_name[5] == '1') return cpuid_ext_info.features.sse4_1;
        if (feature_name[3] == '4' && feature_name[4] == '_' && feature_name[5] == '2') return cpuid_ext_info.features.sse4_2;
        if (feature_name[3] == '\0') return cpuid_ext_info.features.sse;
        if (feature_name[3] == '2') return cpuid_ext_info.features.sse2;
    }
    
    if (feature_name[0] == 'a' && feature_name[1] == 'v' && feature_name[2] == 'x') {
        if (feature_name[3] == '\0') return cpuid_ext_info.features.avx;
        if (feature_name[3] == '2') return cpuid_ext_info.features.avx2;
        if (feature_name[3] == '5' && feature_name[4] == '1' && feature_name[5] == '2') {
            if (feature_name[6] == 'f') return cpuid_ext_info.features.avx512f;
            if (feature_name[6] == 'd' && feature_name[7] == 'q') return cpuid_ext_info.features.avx512dq;
            if (feature_name[6] == 'b' && feature_name[7] == 'w') return cpuid_ext_info.features.avx512bw;
            if (feature_name[6] == 'v' && feature_name[7] == 'l') return cpuid_ext_info.features.avx512vl;
        }
    }
    
    return 0;
}

const char* cpuid_ext_get_vendor_string(void) {
    return cpuid_ext_info.vendor_string;
}

const char* cpuid_ext_get_brand_string(void) {
    return cpuid_ext_info.brand_string;
}

u32 cpuid_ext_get_signature(void) {
    return cpuid_ext_info.signature;
}

u8 cpuid_ext_get_family(void) {
    return cpuid_ext_info.family;
}

u8 cpuid_ext_get_model(void) {
    return cpuid_ext_info.model;
}

u8 cpuid_ext_get_stepping(void) {
    return cpuid_ext_info.stepping;
}

u8 cpuid_ext_get_brand_id(void) {
    return cpuid_ext_info.brand_id;
}

u8 cpuid_ext_get_cache_line_size(void) {
    return cpuid_ext_info.cache_line_size;
}

u8 cpuid_ext_get_max_logical_processors(void) {
    return cpuid_ext_info.max_logical_processors;
}

u8 cpuid_ext_get_initial_apic_id(void) {
    return cpuid_ext_info.initial_apic_id;
}

u32 cpuid_ext_get_max_basic_leaf(void) {
    return cpuid_ext_info.max_basic_leaf;
}

u32 cpuid_ext_get_max_extended_leaf(void) {
    return cpuid_ext_info.max_extended_leaf;
}

const cpuid_features_t* cpuid_ext_get_features(void) {
    return &cpuid_ext_info.features;
}
