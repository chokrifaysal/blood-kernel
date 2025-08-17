/*
 * cpu_ext.h – x86 CPU instruction set extensions (AVX-512/AMX)
 */

#ifndef CPU_EXT_H
#define CPU_EXT_H

#include "kernel/types.h"

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

/* Core functions */
void cpu_ext_init(void);

/* Support detection */
u8 cpu_ext_is_supported(void);
u8 cpu_ext_is_avx512_supported(void);
u8 cpu_ext_is_amx_supported(void);

/* Feature enablement */
u8 cpu_ext_enable_avx512(void);
u8 cpu_ext_enable_amx(void);
u8 cpu_ext_is_avx512_enabled(void);
u8 cpu_ext_is_amx_enabled(void);

/* Feature information */
u32 cpu_ext_get_avx512_features(void);
u32 cpu_ext_get_amx_features(void);

/* Feature usage tracking */
void cpu_ext_use_feature(u8 feature_id);
u32 cpu_ext_get_feature_usage_count(u8 feature_id);
u64 cpu_ext_get_last_feature_use(u8 feature_id);

/* Individual feature support */
u8 cpu_ext_is_feature_supported(u8 feature_id);
u8 cpu_ext_is_feature_enabled(u8 feature_id);
u32 cpu_ext_get_feature_state_size(u8 feature_id);

/* XSAVE support */
u32 cpu_ext_get_xsave_area_size(void);
u64 cpu_ext_get_xcr0_value(void);
void cpu_ext_save_state(void* buffer);
void cpu_ext_restore_state(const void* buffer);

/* AMX tile management */
void cpu_ext_configure_amx_tile(u8 tile_id, u16 rows, u16 bytes_per_row);
void cpu_ext_release_amx_tiles(void);
u8 cpu_ext_get_num_tiles(void);
u16 cpu_ext_get_tile_bytes_per_row(void);
u16 cpu_ext_get_tile_max_names(void);
u16 cpu_ext_get_tile_max_rows(void);

/* Feature checking */
u8 cpu_ext_check_avx512_feature(u32 feature);
u8 cpu_ext_check_amx_feature(u32 feature);

/* Utilities */
void cpu_ext_clear_feature_statistics(void);
const char* cpu_ext_get_feature_name(u8 feature_id);

#endif
