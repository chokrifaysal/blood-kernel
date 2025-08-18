/*
 * cpu_security.h – x86 CPU security features (CET/MPX/SMEP/SMAP/PKU)
 */

#ifndef CPU_SECURITY_H
#define CPU_SECURITY_H

#include "kernel/types.h"

/* Core functions */
void cpu_security_init(void);

/* Support detection */
u8 cpu_security_is_supported(void);
u8 cpu_security_is_smep_supported(void);
u8 cpu_security_is_smap_supported(void);
u8 cpu_security_is_cet_supported(void);
u8 cpu_security_is_mpx_supported(void);
u8 cpu_security_is_pku_supported(void);

/* SMEP/SMAP control */
u8 cpu_security_enable_smep(void);
u8 cpu_security_enable_smap(void);
u8 cpu_security_is_smep_enabled(void);
u8 cpu_security_is_smap_enabled(void);

/* CET (Control Flow Enforcement Technology) */
u8 cpu_security_enable_cet_user(void);
u8 cpu_security_enable_cet_supervisor(void);
u8 cpu_security_is_cet_user_enabled(void);
u8 cpu_security_is_cet_supervisor_enabled(void);
u64 cpu_security_get_user_cet_config(void);
u64 cpu_security_get_supervisor_cet_config(void);

/* MPX (Memory Protection Extensions) */
u8 cpu_security_enable_mpx(void);
u8 cpu_security_is_mpx_enabled(void);

/* Protection Keys (PKU) */
u8 cpu_security_enable_protection_keys(void);
u8 cpu_security_is_pku_enabled(void);
void cpu_security_set_protection_key(u8 key, u8 access_disable, u8 write_disable);
u32 cpu_security_get_pkru_value(void);

/* Violation handling */
void cpu_security_handle_smep_violation(u64 fault_address);
void cpu_security_handle_smap_violation(u64 fault_address);
void cpu_security_handle_mpx_violation(u64 fault_address);
void cpu_security_handle_pkey_violation(u64 fault_address, u8 key);

/* Statistics */
u32 cpu_security_get_total_violations(void);
u32 cpu_security_get_smep_violations(void);
u32 cpu_security_get_smap_violations(void);
u32 cpu_security_get_mpx_violations(void);
u32 cpu_security_get_pkey_violations(void);
u64 cpu_security_get_last_security_event(void);
u32 cpu_security_get_active_mitigations(void);

/* Utilities */
void cpu_security_clear_statistics(void);

#endif
