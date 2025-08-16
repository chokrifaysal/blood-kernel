/*
 * security.h – x86 CPU security features (CET, SMEP/SMAP, MPX)
 */

#ifndef SECURITY_H
#define SECURITY_H

#include "kernel/types.h"

/* Core functions */
void security_init(void);

/* Feature support detection */
u8 security_is_smep_supported(void);
u8 security_is_smap_supported(void);
u8 security_is_cet_supported(void);
u8 security_is_mpx_supported(void);
u8 security_is_pku_supported(void);

/* Feature status */
u8 security_is_smep_enabled(void);
u8 security_is_smap_enabled(void);
u8 security_is_cet_enabled(void);
u8 security_is_mpx_enabled(void);
u8 security_is_pku_enabled(void);

/* CET (Control Flow Enforcement Technology) */
void security_enable_cet(u8 enable_shadow_stack, u8 enable_ibt);
void security_disable_cet(void);
u8 security_is_shadow_stack_supported(void);
u8 security_is_ibt_supported(void);
void security_setup_shadow_stack(u64 ssp_base, u32 size);
u64 security_get_supervisor_cet_config(void);
u64 security_get_user_cet_config(void);

/* MPX (Memory Protection Extensions) */
void security_enable_mpx(void);
void security_disable_mpx(void);
u64 security_get_mpx_config(void);

/* PKU (Protection Key for Userspace) */
void security_set_protection_key(u8 key, u8 access_disable, u8 write_disable);
u32 security_get_pkru(void);

/* SMAP helper functions */
void security_stac(void);  /* Set AC flag */
void security_clac(void);  /* Clear AC flag */

/* Violation handlers */
void security_handle_cet_violation(u32 error_code);
void security_handle_mpx_violation(void);
u8 security_validate_indirect_branch(u64 target_address);

#endif
