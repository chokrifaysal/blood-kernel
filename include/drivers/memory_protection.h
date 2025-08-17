/*
 * memory_protection.h – x86 advanced memory protection (SMEP/SMAP extensions)
 */

#ifndef MEMORY_PROTECTION_H
#define MEMORY_PROTECTION_H

#include "kernel/types.h"

/* Memory protection violation types */
#define VIOLATION_SMEP          1
#define VIOLATION_SMAP          2
#define VIOLATION_PKU           3
#define VIOLATION_CET_SS        4
#define VIOLATION_CET_IBT       5

typedef struct {
    u32 violation_type;
    u64 fault_address;
    u32 error_code;
    u64 instruction_pointer;
    u32 protection_key;
    u64 timestamp;
} protection_violation_t;

/* Core functions */
void memory_protection_init(void);

/* Support detection */
u8 memory_protection_is_supported(void);
u8 memory_protection_is_smep_supported(void);
u8 memory_protection_is_smap_supported(void);
u8 memory_protection_is_pku_supported(void);
u8 memory_protection_is_cet_supported(void);

/* Feature status */
u8 memory_protection_is_smep_enabled(void);
u8 memory_protection_is_smap_enabled(void);
u8 memory_protection_is_pku_enabled(void);
u8 memory_protection_is_cet_enabled(void);
u8 memory_protection_is_shadow_stack_enabled(void);
u8 memory_protection_is_indirect_branch_tracking_enabled(void);

/* SMAP control */
void memory_protection_stac(void);
void memory_protection_clac(void);
void memory_protection_enable_supervisor_access(void);
void memory_protection_disable_supervisor_access(void);
u8 memory_protection_is_supervisor_access_enabled(void);

/* Protection keys */
u8 memory_protection_allocate_protection_key(void);
void memory_protection_free_protection_key(u8 key);
void memory_protection_set_protection_key_rights(u8 key, u8 access_disable, u8 write_disable);
u32 memory_protection_get_pkru(void);
void memory_protection_set_pkru(u32 pkru);

/* Shadow stack */
void memory_protection_setup_shadow_stack(u64 stack_base, u32 stack_size);
u64 memory_protection_get_shadow_stack_pointer(void);

/* CET instructions */
void memory_protection_endbr64(void);
void memory_protection_endbr32(void);

/* Violation handling */
void memory_protection_handle_violation(u32 violation_type, u64 fault_address, u32 error_code, u64 instruction_pointer);
u32 memory_protection_get_violation_count(void);
const protection_violation_t* memory_protection_get_violation(u32 index);
void memory_protection_clear_violations(void);

/* Validation */
u8 memory_protection_validate_user_pointer(const void* ptr, u32 size);

/* Feature control */
void memory_protection_disable_smep(void);
void memory_protection_disable_smap(void);

#endif
