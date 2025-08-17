/*
 * security_ext.h – x86 CPU security features (MPX/CET extensions)
 */

#ifndef SECURITY_EXT_H
#define SECURITY_EXT_H

#include "kernel/types.h"

/* Security violation types */
#define SECURITY_VIOLATION_MPX      1
#define SECURITY_VIOLATION_CET_SS   2
#define SECURITY_VIOLATION_CET_IBT  3
#define SECURITY_VIOLATION_SMEP     4
#define SECURITY_VIOLATION_SMAP     5
#define SECURITY_VIOLATION_PKU      6

typedef struct {
    u64 lower_bound;
    u64 upper_bound;
    u8 enabled;
    u8 preserve;
} mpx_bound_t;

typedef struct {
    u32 violation_type;
    u64 fault_address;
    u64 instruction_pointer;
    u32 error_code;
    u64 timestamp;
} security_violation_t;

/* Core functions */
void security_ext_init(void);

/* Support detection */
u8 security_ext_is_supported(void);
u8 security_ext_is_mpx_supported(void);
u8 security_ext_is_cet_supported(void);

/* Feature status */
u8 security_ext_is_mpx_enabled(void);
u8 security_ext_is_cet_enabled(void);
u8 security_ext_is_shadow_stack_enabled(void);
u8 security_ext_is_ibt_enabled(void);
u8 security_ext_is_nx_enabled(void);

/* Shadow stack management */
void security_ext_setup_shadow_stack(u64 base, u64 size);
u64 security_ext_get_shadow_stack_pointer(void);
void security_ext_setup_interrupt_ssp_table(u64 table_address);
u64 security_ext_get_shadow_stack_base(void);
u64 security_ext_get_shadow_stack_size(void);
u64 security_ext_get_interrupt_ssp_table(void);

/* MPX bounds management */
void security_ext_set_mpx_bound(u8 index, u64 lower, u64 upper);
const mpx_bound_t* security_ext_get_mpx_bound(u8 index);
void security_ext_clear_mpx_bound(u8 index);
u8 security_ext_check_mpx_bounds(u64 address, u8 bound_index);
void security_ext_set_mpx_preserve(u8 preserve);
u8 security_ext_is_mpx_preserve_enabled(void);

/* CET instructions */
void security_ext_endbr64(void);
void security_ext_endbr32(void);

/* Violation handling */
void security_ext_handle_violation(u32 violation_type, u64 fault_address, u64 rip, u32 error_code);
u32 security_ext_get_violation_count(void);
const security_violation_t* security_ext_get_violation(u32 index);
u32 security_ext_get_mpx_violations(void);
u32 security_ext_get_cet_violations(void);
u32 security_ext_get_smep_violations(void);
u32 security_ext_get_smap_violations(void);
void security_ext_clear_violations(void);

/* Feature control */
void security_ext_disable_mpx(void);
void security_ext_disable_cet(void);

#endif
