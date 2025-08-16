/*
 * xsave.h – x86 XSAVE/XRSTOR extended state management
 */

#ifndef XSAVE_H
#define XSAVE_H

#include "kernel/types.h"

/* XSAVE feature bits */
#define XFEATURE_FP             0x01    /* x87 floating point */
#define XFEATURE_SSE            0x02    /* SSE */
#define XFEATURE_YMM            0x04    /* AVX */
#define XFEATURE_BNDREGS        0x08    /* MPX bound registers */
#define XFEATURE_BNDCSR         0x10    /* MPX bound config/status */
#define XFEATURE_OPMASK         0x20    /* AVX-512 opmask */
#define XFEATURE_ZMM_Hi256      0x40    /* AVX-512 upper 256 bits */
#define XFEATURE_Hi16_ZMM       0x80    /* AVX-512 ZMM16-31 */
#define XFEATURE_PKRU           0x200   /* Protection Key Rights */

/* Core functions */
void xsave_init(void);

/* Support detection */
u8 xsave_is_supported(void);
u8 xsave_is_xsaveopt_supported(void);
u8 xsave_is_xsavec_supported(void);
u8 xsave_is_xsaves_supported(void);

/* Feature management */
u64 xsave_get_supported_features(void);
u64 xsave_get_enabled_features(void);
u8 xsave_is_feature_enabled(u32 feature_bit);
u8 xsave_enable_feature(u32 feature_bit);
u8 xsave_disable_feature(u32 feature_bit);

/* Area management */
u32 xsave_get_area_size(void);
u32 xsave_get_feature_offset(u32 feature_bit);
u32 xsave_get_feature_size(u32 feature_bit);
void* xsave_alloc_area(void);
void xsave_init_area(void* xsave_area);

/* State save/restore */
void xsave_save_state(void* xsave_area, u64 feature_mask);
void xsave_restore_state(const void* xsave_area, u64 feature_mask);
void xsave_save_state_compact(void* xsave_area, u64 feature_mask);
void xsave_save_state_supervisor(void* xsave_area, u64 feature_mask);
void xsave_restore_state_supervisor(const void* xsave_area, u64 feature_mask);

/* Utility functions */
u8 xsave_is_supervisor_feature(u32 feature_bit);

#endif
