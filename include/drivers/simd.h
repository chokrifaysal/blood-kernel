/*
 * simd.h – x86 SIMD instruction set extensions (SSE/AVX optimization)
 */

#ifndef SIMD_H
#define SIMD_H

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

/* Core functions */
void simd_init(void);

/* Support detection */
u8 simd_is_supported(u32 instruction_set);
u8 simd_is_enabled(u32 instruction_set);
u32 simd_get_vector_width(void);
u8 simd_is_optimization_enabled(void);

/* Optimized operations */
void simd_memcpy(void* dest, const void* src, u32 size);
void simd_memset(void* dest, u8 value, u32 size);
s32 simd_strcmp(const char* str1, const char* str2);
u32 simd_checksum(const void* data, u32 size);

/* State management */
void simd_save_state(void* state_area);
void simd_restore_state(const void* state_area);
u32 simd_get_state_size(void);

#endif
