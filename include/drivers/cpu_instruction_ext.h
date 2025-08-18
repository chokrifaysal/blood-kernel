/*
 * cpu_instruction_ext.h – x86 CPU instruction set extensions (AVX-512/AMX/APX)
 */

#ifndef CPU_INSTRUCTION_EXT_H
#define CPU_INSTRUCTION_EXT_H

#include "kernel/types.h"

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

/* Core functions */
void cpu_instruction_ext_init(void);

/* Support detection */
u8 cpu_instruction_ext_is_supported(void);
u8 cpu_instruction_ext_is_avx512_supported(void);
u8 cpu_instruction_ext_is_amx_supported(void);
u8 cpu_instruction_ext_is_apx_supported(void);

/* Extension control */
u8 cpu_instruction_ext_enable_avx512(void);
u8 cpu_instruction_ext_enable_amx(void);
u8 cpu_instruction_ext_enable_apx(void);
u8 cpu_instruction_ext_is_avx512_enabled(void);
u8 cpu_instruction_ext_is_amx_enabled(void);
u8 cpu_instruction_ext_is_apx_enabled(void);

/* Feature information */
u32 cpu_instruction_ext_get_avx512_features(void);
u32 cpu_instruction_ext_get_amx_features(void);
u32 cpu_instruction_ext_get_apx_features(void);
u64 cpu_instruction_ext_get_xfeatures_supported(void);
u64 cpu_instruction_ext_get_xfeatures_enabled(void);
u32 cpu_instruction_ext_get_xsave_area_size(void);

/* Context management */
void cpu_instruction_ext_xsave_context(void* save_area);
void cpu_instruction_ext_xrstor_context(const void* save_area);

/* AMX tile configuration */
void cpu_instruction_ext_configure_amx_tiles(u8 num_tiles, u16 tile_size);
u8 cpu_instruction_ext_get_tile_registers_configured(void);
u64 cpu_instruction_ext_get_tile_data_size(void);

/* Usage tracking */
void cpu_instruction_ext_handle_instruction_usage(u8 extension_type);
const isa_extension_t* cpu_instruction_ext_get_extension_info(u8 extension_type);

/* Statistics */
u32 cpu_instruction_ext_get_total_context_switches(void);
u32 cpu_instruction_ext_get_xsave_operations(void);
u32 cpu_instruction_ext_get_xrstor_operations(void);
u32 cpu_instruction_ext_get_avx512_instructions_executed(void);
u32 cpu_instruction_ext_get_amx_instructions_executed(void);
u32 cpu_instruction_ext_get_apx_instructions_executed(void);
u64 cpu_instruction_ext_get_last_context_switch(void);

/* Utilities */
const char* cpu_instruction_ext_get_extension_name(u8 extension_type);
void cpu_instruction_ext_clear_statistics(void);

#endif
