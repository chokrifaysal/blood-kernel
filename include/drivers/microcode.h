/*
 * microcode.h – x86 CPU microcode update support
 */

#ifndef MICROCODE_H
#define MICROCODE_H

#include "kernel/types.h"

/* Core functions */
void microcode_init(void);
u8 microcode_is_supported(void);

/* Vendor detection */
u8 microcode_is_intel(void);
u8 microcode_is_amd(void);

/* Microcode information */
u32 microcode_get_revision(void);
u32 microcode_get_processor_signature(void);
u32 microcode_get_processor_flags(void);

/* Update management */
u8 microcode_load_update(const void* update_data, u32 size);
u8 microcode_apply_update(void);
u8 microcode_is_update_available(void);
u32 microcode_get_update_revision(void);

/* Utility functions */
void microcode_refresh_revision(void);

#endif
