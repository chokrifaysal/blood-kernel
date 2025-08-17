/*
 * cpu_ext.c – x86 CPU instruction set extensions (AVX-512/AMX)
 */

#include "kernel/types.h"

/* XFEATURE bits */
#define XFEATURE_X87                (1 << 0)
#define XFEATURE_SSE                (1 << 1)
#define XFEATURE_AVX                (1 << 2)
#define XFEATURE_BNDREGS            (1 << 3)
#define XFEATURE_BNDCSR             (1 << 4)
#define XFEATURE_OPMASK             (1 << 5)
#define XFEATURE_ZMM_Hi256          (1 << 6)
#define XFEATURE_Hi16_ZMM           (1 << 7)
#define XFEATURE_PT                 (1 << 8)
#define XFEATURE_PKRU               (1 << 9)
#define XFEATURE_PASID              (1 << 10)
#define XFEATURE_CET_U              (1 << 11)
#define XFEATURE_CET_S              (1 << 12)
#define XFEATURE_HDC                (1 << 13)
#define XFEATURE_UINTR              (1 << 14)
#define XFEATURE_LBR                (1 << 15)
#define XFEATURE_HWP                (1 << 16)
#define XFEATURE_XTILECFG           (1 << 17)
#define XFEATURE_XTILEDATA          (1 << 18)

/* AVX-512 instruction sets */
#define AVX512_F                    (1 << 16)
#define AVX512_DQ                   (1 << 17)
#define AVX512_IFMA                 (1 << 21)
#define AVX512_PF                   (1 << 26)
#define AVX512_ER                   (1 << 27)
#define AVX512_CD                   (1 << 28)
#define AVX512_BW                   (1 << 30)
#define AVX512_VL                   (1 << 31)

/* AMX features */
#define AMX_BF16                    (1 << 22)
#define AMX_TILE                    (1 << 24)
#define AMX_INT8                    (1 << 25)

typedef struct {
    u8 supported;
    u8 enabled;
    u32 state_size;
    u64 state_mask;
} cpu_feature_t;

typedef struct {
    u8 cpu_ext_supported;
    u8 avx512_supported;
    u8 amx_supported;
    u8 avx512_enabled;
    u8 amx_enabled;
    u32 avx512_features;
    u32 amx_features;
    u64 xcr0_value;
    u64 xss_value;
    u32 xsave_area_size;
    cpu_feature_t features[32];
    u32 feature_usage_count[32];
    u64 last_feature_use[32];
    u8 tile_config[64];
    u8 num_tiles;
    u16 tile_bytes_per_row;
    u16 tile_max_names;
    u16 tile_max_rows;
} cpu_ext_info_t;

static cpu_ext_info_t cpu_ext_info;

extern u64 timer_get_ticks(void);

static void cpu_ext_detect_avx512(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check for AVX-512 foundation */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    
    if (ebx & AVX512_F) {
        cpu_ext_info.avx512_supported = 1;
        cpu_ext_info.avx512_features = ebx;
        
        /* Check additional AVX-512 features */
        if (ebx & AVX512_DQ) cpu_ext_info.avx512_features |= AVX512_DQ;
        if (ebx & AVX512_IFMA) cpu_ext_info.avx512_features |= AVX512_IFMA;
        if (ebx & AVX512_PF) cpu_ext_info.avx512_features |= AVX512_PF;
        if (ebx & AVX512_ER) cpu_ext_info.avx512_features |= AVX512_ER;
        if (ebx & AVX512_CD) cpu_ext_info.avx512_features |= AVX512_CD;
        if (ebx & AVX512_BW) cpu_ext_info.avx512_features |= AVX512_BW;
        if (ebx & AVX512_VL) cpu_ext_info.avx512_features |= AVX512_VL;
    }
}

static void cpu_ext_detect_amx(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check for AMX support */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    
    if (edx & AMX_TILE) {
        cpu_ext_info.amx_supported = 1;
        cpu_ext_info.amx_features = edx;
        
        if (edx & AMX_BF16) cpu_ext_info.amx_features |= AMX_BF16;
        if (edx & AMX_INT8) cpu_ext_info.amx_features |= AMX_INT8;
        
        /* Get AMX tile configuration */
        __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x1D), "c"(1));
        cpu_ext_info.num_tiles = eax & 0xFF;
        cpu_ext_info.tile_bytes_per_row = (eax >> 16) & 0xFFFF;
        cpu_ext_info.tile_max_names = ebx & 0xFFFF;
        cpu_ext_info.tile_max_rows = (ebx >> 16) & 0xFFFF;
    }
}

static void cpu_ext_setup_xsave_features(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Get XSAVE supported features */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0xD), "c"(0));
    
    u64 supported_features = ((u64)edx << 32) | eax;
    cpu_ext_info.xsave_area_size = ecx;
    
    /* Setup individual features */
    for (u8 i = 0; i < 32; i++) {
        if (supported_features & (1ULL << i)) {
            cpu_ext_info.features[i].supported = 1;
            
            /* Get feature-specific information */
            __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0xD), "c"(i));
            cpu_ext_info.features[i].state_size = eax;
            cpu_ext_info.features[i].state_mask = 1ULL << i;
        }
    }
}

void cpu_ext_init(void) {
    cpu_ext_info.cpu_ext_supported = 0;
    cpu_ext_info.avx512_supported = 0;
    cpu_ext_info.amx_supported = 0;
    cpu_ext_info.avx512_enabled = 0;
    cpu_ext_info.amx_enabled = 0;
    cpu_ext_info.avx512_features = 0;
    cpu_ext_info.amx_features = 0;
    cpu_ext_info.xcr0_value = 0;
    cpu_ext_info.xss_value = 0;
    cpu_ext_info.xsave_area_size = 0;
    cpu_ext_info.num_tiles = 0;
    cpu_ext_info.tile_bytes_per_row = 0;
    cpu_ext_info.tile_max_names = 0;
    cpu_ext_info.tile_max_rows = 0;
    
    for (u8 i = 0; i < 32; i++) {
        cpu_ext_info.features[i].supported = 0;
        cpu_ext_info.features[i].enabled = 0;
        cpu_ext_info.features[i].state_size = 0;
        cpu_ext_info.features[i].state_mask = 0;
        cpu_ext_info.feature_usage_count[i] = 0;
        cpu_ext_info.last_feature_use[i] = 0;
    }
    
    for (u8 i = 0; i < 64; i++) {
        cpu_ext_info.tile_config[i] = 0;
    }
    
    cpu_ext_detect_avx512();
    cpu_ext_detect_amx();
    cpu_ext_setup_xsave_features();
    
    if (cpu_ext_info.avx512_supported || cpu_ext_info.amx_supported) {
        cpu_ext_info.cpu_ext_supported = 1;
    }
}

u8 cpu_ext_is_supported(void) {
    return cpu_ext_info.cpu_ext_supported;
}

u8 cpu_ext_is_avx512_supported(void) {
    return cpu_ext_info.avx512_supported;
}

u8 cpu_ext_is_amx_supported(void) {
    return cpu_ext_info.amx_supported;
}

u8 cpu_ext_enable_avx512(void) {
    if (!cpu_ext_info.avx512_supported) return 0;
    
    /* Enable AVX-512 in XCR0 */
    u64 xcr0 = XFEATURE_X87 | XFEATURE_SSE | XFEATURE_AVX | 
               XFEATURE_OPMASK | XFEATURE_ZMM_Hi256 | XFEATURE_Hi16_ZMM;
    
    __asm__ volatile("xsetbv" : : "a"((u32)xcr0), "d"((u32)(xcr0 >> 32)), "c"(0));
    
    cpu_ext_info.xcr0_value = xcr0;
    cpu_ext_info.avx512_enabled = 1;
    
    /* Enable individual AVX-512 features */
    cpu_ext_info.features[5].enabled = 1; /* OPMASK */
    cpu_ext_info.features[6].enabled = 1; /* ZMM_Hi256 */
    cpu_ext_info.features[7].enabled = 1; /* Hi16_ZMM */
    
    return 1;
}

u8 cpu_ext_enable_amx(void) {
    if (!cpu_ext_info.amx_supported) return 0;
    
    /* Enable AMX in XCR0 */
    u64 xcr0 = cpu_ext_info.xcr0_value | XFEATURE_XTILECFG | XFEATURE_XTILEDATA;
    
    __asm__ volatile("xsetbv" : : "a"((u32)xcr0), "d"((u32)(xcr0 >> 32)), "c"(0));
    
    cpu_ext_info.xcr0_value = xcr0;
    cpu_ext_info.amx_enabled = 1;
    
    /* Enable AMX features */
    cpu_ext_info.features[17].enabled = 1; /* XTILECFG */
    cpu_ext_info.features[18].enabled = 1; /* XTILEDATA */
    
    return 1;
}

u8 cpu_ext_is_avx512_enabled(void) {
    return cpu_ext_info.avx512_enabled;
}

u8 cpu_ext_is_amx_enabled(void) {
    return cpu_ext_info.amx_enabled;
}

u32 cpu_ext_get_avx512_features(void) {
    return cpu_ext_info.avx512_features;
}

u32 cpu_ext_get_amx_features(void) {
    return cpu_ext_info.amx_features;
}

void cpu_ext_use_feature(u8 feature_id) {
    if (feature_id >= 32) return;
    
    cpu_ext_info.feature_usage_count[feature_id]++;
    cpu_ext_info.last_feature_use[feature_id] = timer_get_ticks();
}

u32 cpu_ext_get_feature_usage_count(u8 feature_id) {
    if (feature_id >= 32) return 0;
    return cpu_ext_info.feature_usage_count[feature_id];
}

u64 cpu_ext_get_last_feature_use(u8 feature_id) {
    if (feature_id >= 32) return 0;
    return cpu_ext_info.last_feature_use[feature_id];
}

u8 cpu_ext_is_feature_supported(u8 feature_id) {
    if (feature_id >= 32) return 0;
    return cpu_ext_info.features[feature_id].supported;
}

u8 cpu_ext_is_feature_enabled(u8 feature_id) {
    if (feature_id >= 32) return 0;
    return cpu_ext_info.features[feature_id].enabled;
}

u32 cpu_ext_get_feature_state_size(u8 feature_id) {
    if (feature_id >= 32) return 0;
    return cpu_ext_info.features[feature_id].state_size;
}

u32 cpu_ext_get_xsave_area_size(void) {
    return cpu_ext_info.xsave_area_size;
}

u64 cpu_ext_get_xcr0_value(void) {
    return cpu_ext_info.xcr0_value;
}

void cpu_ext_save_state(void* buffer) {
    if (!cpu_ext_info.cpu_ext_supported) return;
    
    __asm__ volatile("xsave %0" : "=m"(*(char*)buffer) : "a"(0xFFFFFFFF), "d"(0xFFFFFFFF));
}

void cpu_ext_restore_state(const void* buffer) {
    if (!cpu_ext_info.cpu_ext_supported) return;
    
    __asm__ volatile("xrstor %0" : : "m"(*(const char*)buffer), "a"(0xFFFFFFFF), "d"(0xFFFFFFFF));
}

void cpu_ext_configure_amx_tile(u8 tile_id, u16 rows, u16 bytes_per_row) {
    if (!cpu_ext_info.amx_enabled || tile_id >= cpu_ext_info.num_tiles) return;
    
    /* Configure tile in tile configuration */
    u8* config = &cpu_ext_info.tile_config[tile_id * 2];
    config[0] = rows;
    config[1] = bytes_per_row;
    
    /* Load tile configuration */
    __asm__ volatile("ldtilecfg %0" : : "m"(cpu_ext_info.tile_config));
}

void cpu_ext_release_amx_tiles(void) {
    if (!cpu_ext_info.amx_enabled) return;
    
    __asm__ volatile("tilerelease");
    
    /* Clear tile configuration */
    for (u8 i = 0; i < 64; i++) {
        cpu_ext_info.tile_config[i] = 0;
    }
}

u8 cpu_ext_get_num_tiles(void) {
    return cpu_ext_info.num_tiles;
}

u16 cpu_ext_get_tile_bytes_per_row(void) {
    return cpu_ext_info.tile_bytes_per_row;
}

u16 cpu_ext_get_tile_max_names(void) {
    return cpu_ext_info.tile_max_names;
}

u16 cpu_ext_get_tile_max_rows(void) {
    return cpu_ext_info.tile_max_rows;
}

u8 cpu_ext_check_avx512_feature(u32 feature) {
    return (cpu_ext_info.avx512_features & feature) != 0;
}

u8 cpu_ext_check_amx_feature(u32 feature) {
    return (cpu_ext_info.amx_features & feature) != 0;
}

void cpu_ext_clear_feature_statistics(void) {
    for (u8 i = 0; i < 32; i++) {
        cpu_ext_info.feature_usage_count[i] = 0;
        cpu_ext_info.last_feature_use[i] = 0;
    }
}

const char* cpu_ext_get_feature_name(u8 feature_id) {
    switch (feature_id) {
        case 0: return "x87";
        case 1: return "SSE";
        case 2: return "AVX";
        case 3: return "BNDREGS";
        case 4: return "BNDCSR";
        case 5: return "OPMASK";
        case 6: return "ZMM_Hi256";
        case 7: return "Hi16_ZMM";
        case 8: return "PT";
        case 9: return "PKRU";
        case 10: return "PASID";
        case 11: return "CET_U";
        case 12: return "CET_S";
        case 13: return "HDC";
        case 14: return "UINTR";
        case 15: return "LBR";
        case 16: return "HWP";
        case 17: return "XTILECFG";
        case 18: return "XTILEDATA";
        default: return "Unknown";
    }
}
