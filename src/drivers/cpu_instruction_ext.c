/*
 * cpu_instruction_ext.c – x86 CPU instruction set extensions (AVX-512/AMX/APX)
 */

#include "kernel/types.h"

/* XFEATURE bits for instruction set extensions */
#define XFEATURE_YMM                    (1ULL << 2)
#define XFEATURE_BNDREGS                (1ULL << 3)
#define XFEATURE_BNDCSR                 (1ULL << 4)
#define XFEATURE_OPMASK                 (1ULL << 5)
#define XFEATURE_ZMM_Hi256              (1ULL << 6)
#define XFEATURE_Hi16_ZMM               (1ULL << 7)
#define XFEATURE_PT                     (1ULL << 8)
#define XFEATURE_PKRU                   (1ULL << 9)
#define XFEATURE_PASID                  (1ULL << 10)
#define XFEATURE_CET_U                  (1ULL << 11)
#define XFEATURE_CET_S                  (1ULL << 12)
#define XFEATURE_HDC                    (1ULL << 13)
#define XFEATURE_UINTR                  (1ULL << 14)
#define XFEATURE_LBR                    (1ULL << 15)
#define XFEATURE_HWP                    (1ULL << 16)
#define XFEATURE_XTILECFG               (1ULL << 17)
#define XFEATURE_XTILEDATA              (1ULL << 18)
#define XFEATURE_APX_F                  (1ULL << 19)

/* AVX-512 feature flags */
#define AVX512_F                        (1 << 16)
#define AVX512_DQ                       (1 << 17)
#define AVX512_IFMA                     (1 << 21)
#define AVX512_PF                       (1 << 26)
#define AVX512_ER                       (1 << 27)
#define AVX512_CD                       (1 << 28)
#define AVX512_BW                       (1 << 30)
#define AVX512_VL                       (1 << 31)

/* AMX feature flags */
#define AMX_BF16                        (1 << 22)
#define AMX_TILE                        (1 << 24)
#define AMX_INT8                        (1 << 25)

/* APX feature flags */
#define APX_F                           (1 << 21)

/* Instruction set extension types */
#define ISA_EXT_SSE                     0
#define ISA_EXT_SSE2                    1
#define ISA_EXT_SSE3                    2
#define ISA_EXT_SSSE3                   3
#define ISA_EXT_SSE41                   4
#define ISA_EXT_SSE42                   5
#define ISA_EXT_AVX                     6
#define ISA_EXT_AVX2                    7
#define ISA_EXT_AVX512F                 8
#define ISA_EXT_AVX512DQ                9
#define ISA_EXT_AVX512BW                10
#define ISA_EXT_AVX512VL                11
#define ISA_EXT_AMX_TILE                12
#define ISA_EXT_AMX_INT8                13
#define ISA_EXT_AMX_BF16                14
#define ISA_EXT_APX_F                   15

typedef struct {
    u8 extension_type;
    u8 supported;
    u8 enabled;
    u32 usage_count;
    u64 last_used;
    u32 context_switches;
} isa_extension_t;

typedef struct {
    u8 avx512_supported;
    u8 avx512_enabled;
    u32 avx512_features;
    u8 zmm_registers_used;
    u8 opmask_registers_used;
    u32 avx512_instructions_executed;
} avx512_info_t;

typedef struct {
    u8 amx_supported;
    u8 amx_enabled;
    u32 amx_features;
    u8 tile_registers_configured;
    u32 tile_config_changes;
    u32 amx_instructions_executed;
    u64 tile_data_size;
} amx_info_t;

typedef struct {
    u8 apx_supported;
    u8 apx_enabled;
    u32 apx_features;
    u8 extended_gpr_used;
    u32 apx_instructions_executed;
} apx_info_t;

typedef struct {
    u8 cpu_instruction_ext_supported;
    u64 xfeatures_supported;
    u64 xfeatures_enabled;
    u32 xsave_area_size;
    isa_extension_t extensions[16];
    avx512_info_t avx512;
    amx_info_t amx;
    apx_info_t apx;
    u32 total_context_switches;
    u32 xsave_operations;
    u32 xrstor_operations;
    u64 last_context_switch;
} cpu_instruction_ext_info_t;

static cpu_instruction_ext_info_t cpu_inst_ext_info;

extern u64 timer_get_ticks(void);

static void cpu_instruction_ext_detect_avx512(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check for AVX-512 support */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    
    cpu_inst_ext_info.avx512.avx512_features = 0;
    
    if (ebx & AVX512_F) {
        cpu_inst_ext_info.avx512.avx512_features |= AVX512_F;
        cpu_inst_ext_info.extensions[ISA_EXT_AVX512F].supported = 1;
    }
    if (ebx & AVX512_DQ) {
        cpu_inst_ext_info.avx512.avx512_features |= AVX512_DQ;
        cpu_inst_ext_info.extensions[ISA_EXT_AVX512DQ].supported = 1;
    }
    if (ebx & AVX512_BW) {
        cpu_inst_ext_info.avx512.avx512_features |= AVX512_BW;
        cpu_inst_ext_info.extensions[ISA_EXT_AVX512BW].supported = 1;
    }
    if (ebx & AVX512_VL) {
        cpu_inst_ext_info.avx512.avx512_features |= AVX512_VL;
        cpu_inst_ext_info.extensions[ISA_EXT_AVX512VL].supported = 1;
    }
    
    cpu_inst_ext_info.avx512.avx512_supported = (cpu_inst_ext_info.avx512.avx512_features & AVX512_F) != 0;
}

static void cpu_instruction_ext_detect_amx(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check for AMX support */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    
    cpu_inst_ext_info.amx.amx_features = 0;
    
    if (edx & AMX_BF16) {
        cpu_inst_ext_info.amx.amx_features |= AMX_BF16;
        cpu_inst_ext_info.extensions[ISA_EXT_AMX_BF16].supported = 1;
    }
    if (edx & AMX_TILE) {
        cpu_inst_ext_info.amx.amx_features |= AMX_TILE;
        cpu_inst_ext_info.extensions[ISA_EXT_AMX_TILE].supported = 1;
    }
    if (edx & AMX_INT8) {
        cpu_inst_ext_info.amx.amx_features |= AMX_INT8;
        cpu_inst_ext_info.extensions[ISA_EXT_AMX_INT8].supported = 1;
    }
    
    cpu_inst_ext_info.amx.amx_supported = (cpu_inst_ext_info.amx.amx_features & AMX_TILE) != 0;
}

static void cpu_instruction_ext_detect_apx(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check for APX support */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(1));
    
    cpu_inst_ext_info.apx.apx_features = 0;
    
    if (edx & APX_F) {
        cpu_inst_ext_info.apx.apx_features |= APX_F;
        cpu_inst_ext_info.extensions[ISA_EXT_APX_F].supported = 1;
    }
    
    cpu_inst_ext_info.apx.apx_supported = (cpu_inst_ext_info.apx.apx_features & APX_F) != 0;
}

static void cpu_instruction_ext_detect_xfeatures(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Get XSAVE supported features */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0xD), "c"(0));
    
    cpu_inst_ext_info.xfeatures_supported = ((u64)edx << 32) | eax;
    cpu_inst_ext_info.xsave_area_size = ecx;
    
    /* Get current XCR0 value */
    u64 xcr0;
    __asm__ volatile("xgetbv" : "=a"((u32)xcr0), "=d"((u32)(xcr0 >> 32)) : "c"(0));
    cpu_inst_ext_info.xfeatures_enabled = xcr0;
}

void cpu_instruction_ext_init(void) {
    cpu_inst_ext_info.cpu_instruction_ext_supported = 0;
    cpu_inst_ext_info.xfeatures_supported = 0;
    cpu_inst_ext_info.xfeatures_enabled = 0;
    cpu_inst_ext_info.xsave_area_size = 0;
    cpu_inst_ext_info.total_context_switches = 0;
    cpu_inst_ext_info.xsave_operations = 0;
    cpu_inst_ext_info.xrstor_operations = 0;
    cpu_inst_ext_info.last_context_switch = 0;
    
    /* Initialize extension tracking */
    for (u8 i = 0; i < 16; i++) {
        cpu_inst_ext_info.extensions[i].extension_type = i;
        cpu_inst_ext_info.extensions[i].supported = 0;
        cpu_inst_ext_info.extensions[i].enabled = 0;
        cpu_inst_ext_info.extensions[i].usage_count = 0;
        cpu_inst_ext_info.extensions[i].last_used = 0;
        cpu_inst_ext_info.extensions[i].context_switches = 0;
    }
    
    /* Initialize AVX-512 */
    cpu_inst_ext_info.avx512.avx512_supported = 0;
    cpu_inst_ext_info.avx512.avx512_enabled = 0;
    cpu_inst_ext_info.avx512.avx512_features = 0;
    cpu_inst_ext_info.avx512.zmm_registers_used = 0;
    cpu_inst_ext_info.avx512.opmask_registers_used = 0;
    cpu_inst_ext_info.avx512.avx512_instructions_executed = 0;
    
    /* Initialize AMX */
    cpu_inst_ext_info.amx.amx_supported = 0;
    cpu_inst_ext_info.amx.amx_enabled = 0;
    cpu_inst_ext_info.amx.amx_features = 0;
    cpu_inst_ext_info.amx.tile_registers_configured = 0;
    cpu_inst_ext_info.amx.tile_config_changes = 0;
    cpu_inst_ext_info.amx.amx_instructions_executed = 0;
    cpu_inst_ext_info.amx.tile_data_size = 0;
    
    /* Initialize APX */
    cpu_inst_ext_info.apx.apx_supported = 0;
    cpu_inst_ext_info.apx.apx_enabled = 0;
    cpu_inst_ext_info.apx.apx_features = 0;
    cpu_inst_ext_info.apx.extended_gpr_used = 0;
    cpu_inst_ext_info.apx.apx_instructions_executed = 0;
    
    cpu_instruction_ext_detect_xfeatures();
    cpu_instruction_ext_detect_avx512();
    cpu_instruction_ext_detect_amx();
    cpu_instruction_ext_detect_apx();
    
    if (cpu_inst_ext_info.avx512.avx512_supported || 
        cpu_inst_ext_info.amx.amx_supported || 
        cpu_inst_ext_info.apx.apx_supported) {
        cpu_inst_ext_info.cpu_instruction_ext_supported = 1;
    }
}

u8 cpu_instruction_ext_is_supported(void) {
    return cpu_inst_ext_info.cpu_instruction_ext_supported;
}

u8 cpu_instruction_ext_enable_avx512(void) {
    if (!cpu_inst_ext_info.avx512.avx512_supported) return 0;
    
    /* Enable AVX-512 in XCR0 */
    u64 xcr0 = cpu_inst_ext_info.xfeatures_enabled;
    xcr0 |= XFEATURE_OPMASK | XFEATURE_ZMM_Hi256 | XFEATURE_Hi16_ZMM;
    
    __asm__ volatile("xsetbv" : : "a"((u32)xcr0), "d"((u32)(xcr0 >> 32)), "c"(0));
    
    cpu_inst_ext_info.xfeatures_enabled = xcr0;
    cpu_inst_ext_info.avx512.avx512_enabled = 1;
    cpu_inst_ext_info.extensions[ISA_EXT_AVX512F].enabled = 1;
    
    return 1;
}

u8 cpu_instruction_ext_enable_amx(void) {
    if (!cpu_inst_ext_info.amx.amx_supported) return 0;
    
    /* Enable AMX in XCR0 */
    u64 xcr0 = cpu_inst_ext_info.xfeatures_enabled;
    xcr0 |= XFEATURE_XTILECFG | XFEATURE_XTILEDATA;
    
    __asm__ volatile("xsetbv" : : "a"((u32)xcr0), "d"((u32)(xcr0 >> 32)), "c"(0));
    
    cpu_inst_ext_info.xfeatures_enabled = xcr0;
    cpu_inst_ext_info.amx.amx_enabled = 1;
    cpu_inst_ext_info.extensions[ISA_EXT_AMX_TILE].enabled = 1;
    
    return 1;
}

u8 cpu_instruction_ext_enable_apx(void) {
    if (!cpu_inst_ext_info.apx.apx_supported) return 0;
    
    /* Enable APX in XCR0 */
    u64 xcr0 = cpu_inst_ext_info.xfeatures_enabled;
    xcr0 |= XFEATURE_APX_F;
    
    __asm__ volatile("xsetbv" : : "a"((u32)xcr0), "d"((u32)(xcr0 >> 32)), "c"(0));
    
    cpu_inst_ext_info.xfeatures_enabled = xcr0;
    cpu_inst_ext_info.apx.apx_enabled = 1;
    cpu_inst_ext_info.extensions[ISA_EXT_APX_F].enabled = 1;
    
    return 1;
}

void cpu_instruction_ext_xsave_context(void* save_area) {
    if (!cpu_inst_ext_info.cpu_instruction_ext_supported) return;
    
    u64 mask = cpu_inst_ext_info.xfeatures_enabled;
    __asm__ volatile("xsave64 %0" : "=m"(*(char*)save_area) : "a"((u32)mask), "d"((u32)(mask >> 32)) : "memory");
    
    cpu_inst_ext_info.xsave_operations++;
    cpu_inst_ext_info.total_context_switches++;
    cpu_inst_ext_info.last_context_switch = timer_get_ticks();
}

void cpu_instruction_ext_xrstor_context(const void* save_area) {
    if (!cpu_inst_ext_info.cpu_instruction_ext_supported) return;
    
    u64 mask = cpu_inst_ext_info.xfeatures_enabled;
    __asm__ volatile("xrstor64 %0" : : "m"(*(const char*)save_area), "a"((u32)mask), "d"((u32)(mask >> 32)) : "memory");
    
    cpu_inst_ext_info.xrstor_operations++;
}

void cpu_instruction_ext_configure_amx_tiles(u8 num_tiles, u16 tile_size) {
    if (!cpu_inst_ext_info.amx.amx_enabled) return;
    
    /* Configure tile registers using LDTILECFG */
    struct {
        u8 palette_id;
        u8 start_row;
        u8 reserved[14];
        u16 colsb[16];
        u8 rows[16];
    } tile_config = {0};
    
    tile_config.palette_id = 1;
    for (u8 i = 0; i < num_tiles && i < 8; i++) {
        tile_config.colsb[i] = tile_size;
        tile_config.rows[i] = tile_size / 4;
    }
    
    __asm__ volatile("ldtilecfg %0" : : "m"(tile_config) : "memory");
    
    cpu_inst_ext_info.amx.tile_registers_configured = num_tiles;
    cpu_inst_ext_info.amx.tile_config_changes++;
    cpu_inst_ext_info.amx.tile_data_size = num_tiles * tile_size * tile_size;
}

void cpu_instruction_ext_handle_instruction_usage(u8 extension_type) {
    if (extension_type >= 16) return;
    
    isa_extension_t* ext = &cpu_inst_ext_info.extensions[extension_type];
    ext->usage_count++;
    ext->last_used = timer_get_ticks();
    
    switch (extension_type) {
        case ISA_EXT_AVX512F:
        case ISA_EXT_AVX512DQ:
        case ISA_EXT_AVX512BW:
        case ISA_EXT_AVX512VL:
            cpu_inst_ext_info.avx512.avx512_instructions_executed++;
            break;
        case ISA_EXT_AMX_TILE:
        case ISA_EXT_AMX_INT8:
        case ISA_EXT_AMX_BF16:
            cpu_inst_ext_info.amx.amx_instructions_executed++;
            break;
        case ISA_EXT_APX_F:
            cpu_inst_ext_info.apx.apx_instructions_executed++;
            break;
    }
}

u8 cpu_instruction_ext_is_avx512_supported(void) {
    return cpu_inst_ext_info.avx512.avx512_supported;
}

u8 cpu_instruction_ext_is_avx512_enabled(void) {
    return cpu_inst_ext_info.avx512.avx512_enabled;
}

u8 cpu_instruction_ext_is_amx_supported(void) {
    return cpu_inst_ext_info.amx.amx_supported;
}

u8 cpu_instruction_ext_is_amx_enabled(void) {
    return cpu_inst_ext_info.amx.amx_enabled;
}

u8 cpu_instruction_ext_is_apx_supported(void) {
    return cpu_inst_ext_info.apx.apx_supported;
}

u8 cpu_instruction_ext_is_apx_enabled(void) {
    return cpu_inst_ext_info.apx.apx_enabled;
}

u32 cpu_instruction_ext_get_avx512_features(void) {
    return cpu_inst_ext_info.avx512.avx512_features;
}

u32 cpu_instruction_ext_get_amx_features(void) {
    return cpu_inst_ext_info.amx.amx_features;
}

u32 cpu_instruction_ext_get_apx_features(void) {
    return cpu_inst_ext_info.apx.apx_features;
}

u64 cpu_instruction_ext_get_xfeatures_supported(void) {
    return cpu_inst_ext_info.xfeatures_supported;
}

u64 cpu_instruction_ext_get_xfeatures_enabled(void) {
    return cpu_inst_ext_info.xfeatures_enabled;
}

u32 cpu_instruction_ext_get_xsave_area_size(void) {
    return cpu_inst_ext_info.xsave_area_size;
}

const isa_extension_t* cpu_instruction_ext_get_extension_info(u8 extension_type) {
    if (extension_type >= 16) return 0;
    return &cpu_inst_ext_info.extensions[extension_type];
}

u32 cpu_instruction_ext_get_total_context_switches(void) {
    return cpu_inst_ext_info.total_context_switches;
}

u32 cpu_instruction_ext_get_xsave_operations(void) {
    return cpu_inst_ext_info.xsave_operations;
}

u32 cpu_instruction_ext_get_xrstor_operations(void) {
    return cpu_inst_ext_info.xrstor_operations;
}

u32 cpu_instruction_ext_get_avx512_instructions_executed(void) {
    return cpu_inst_ext_info.avx512.avx512_instructions_executed;
}

u32 cpu_instruction_ext_get_amx_instructions_executed(void) {
    return cpu_inst_ext_info.amx.amx_instructions_executed;
}

u32 cpu_instruction_ext_get_apx_instructions_executed(void) {
    return cpu_inst_ext_info.apx.apx_instructions_executed;
}

u8 cpu_instruction_ext_get_tile_registers_configured(void) {
    return cpu_inst_ext_info.amx.tile_registers_configured;
}

u64 cpu_instruction_ext_get_tile_data_size(void) {
    return cpu_inst_ext_info.amx.tile_data_size;
}

u64 cpu_instruction_ext_get_last_context_switch(void) {
    return cpu_inst_ext_info.last_context_switch;
}

const char* cpu_instruction_ext_get_extension_name(u8 extension_type) {
    switch (extension_type) {
        case ISA_EXT_SSE: return "SSE";
        case ISA_EXT_SSE2: return "SSE2";
        case ISA_EXT_SSE3: return "SSE3";
        case ISA_EXT_SSSE3: return "SSSE3";
        case ISA_EXT_SSE41: return "SSE4.1";
        case ISA_EXT_SSE42: return "SSE4.2";
        case ISA_EXT_AVX: return "AVX";
        case ISA_EXT_AVX2: return "AVX2";
        case ISA_EXT_AVX512F: return "AVX-512F";
        case ISA_EXT_AVX512DQ: return "AVX-512DQ";
        case ISA_EXT_AVX512BW: return "AVX-512BW";
        case ISA_EXT_AVX512VL: return "AVX-512VL";
        case ISA_EXT_AMX_TILE: return "AMX-TILE";
        case ISA_EXT_AMX_INT8: return "AMX-INT8";
        case ISA_EXT_AMX_BF16: return "AMX-BF16";
        case ISA_EXT_APX_F: return "APX-F";
        default: return "Unknown";
    }
}

void cpu_instruction_ext_clear_statistics(void) {
    cpu_inst_ext_info.total_context_switches = 0;
    cpu_inst_ext_info.xsave_operations = 0;
    cpu_inst_ext_info.xrstor_operations = 0;
    cpu_inst_ext_info.avx512.avx512_instructions_executed = 0;
    cpu_inst_ext_info.amx.amx_instructions_executed = 0;
    cpu_inst_ext_info.apx.apx_instructions_executed = 0;
    cpu_inst_ext_info.amx.tile_config_changes = 0;
    
    for (u8 i = 0; i < 16; i++) {
        cpu_inst_ext_info.extensions[i].usage_count = 0;
        cpu_inst_ext_info.extensions[i].context_switches = 0;
    }
}
