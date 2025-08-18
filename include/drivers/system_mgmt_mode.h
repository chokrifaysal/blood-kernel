/*
 * system_mgmt_mode.h – x86 System Management Mode (SMM/SMI) management
 */

#ifndef SYSTEM_MGMT_MODE_H
#define SYSTEM_MGMT_MODE_H

#include "kernel/types.h"

/* SMI sources */
#define SMI_SOURCE_TIMER                0
#define SMI_SOURCE_GPIO                 1
#define SMI_SOURCE_LEGACY_USB           2
#define SMI_SOURCE_PERIODIC             3
#define SMI_SOURCE_TCO                  4
#define SMI_SOURCE_MCSMI                5
#define SMI_SOURCE_BIOS_RLS             6
#define SMI_SOURCE_BIOS_EN              7
#define SMI_SOURCE_EOS                  8
#define SMI_SOURCE_GBL_SMI_EN           9
#define SMI_SOURCE_LEGACY_USB2          10
#define SMI_SOURCE_INTEL_USB2           11
#define SMI_SOURCE_SWSMI_TMR            12
#define SMI_SOURCE_APMC                 13
#define SMI_SOURCE_SLP_SMI              14
#define SMI_SOURCE_PM1_STS              15

typedef struct {
    u32 eax, ecx, edx, ebx;
    u32 esp, ebp, esi, edi;
    u32 eip, eflags;
    u32 cr0, cr3, cr4;
    u32 es, cs, ss, ds, fs, gs;
    u32 ldtr, tr;
    u32 dr6, dr7;
    u64 smbase;
    u32 smm_revision;
    u32 auto_halt_restart;
    u64 io_restart_rip;
    u64 io_restart_rcx;
    u64 io_restart_rsi;
    u64 io_restart_rdi;
} smm_state_save_t;

typedef struct {
    u8 smi_source;
    u64 timestamp;
    u32 duration_cycles;
    u32 cpu_id;
    u64 instruction_pointer;
} smi_event_t;

/* Core functions */
void system_mgmt_mode_init(void);

/* Support detection */
u8 system_mgmt_mode_is_supported(void);
u8 system_mgmt_mode_is_enabled(void);
u8 system_mgmt_mode_is_locked(void);

/* SMI source management */
u8 system_mgmt_mode_enable_smi_source(u8 smi_source);
u8 system_mgmt_mode_disable_smi_source(u8 smi_source);
void system_mgmt_mode_trigger_smi(u8 smi_source);

/* SMM handler management */
u8 system_mgmt_mode_install_handler(void* handler_code, u32 handler_size);
u8 system_mgmt_mode_is_handler_installed(void);
u64 system_mgmt_mode_get_handler_address(void);

/* State management */
void system_mgmt_mode_save_state(smm_state_save_t* state);
void system_mgmt_mode_restore_state(smm_state_save_t* state);

/* Event handling */
void system_mgmt_mode_handle_smi_entry(u8 smi_source, u64 instruction_pointer);
void system_mgmt_mode_handle_smi_exit(u32 duration_cycles);

/* SMRAM information */
u64 system_mgmt_mode_get_smram_base(void);
u32 system_mgmt_mode_get_smram_size(void);
u32 system_mgmt_mode_get_smm_revision(void);

/* Statistics */
u32 system_mgmt_mode_get_total_smi_count(void);
u32 system_mgmt_mode_get_smi_sources_enabled(void);
u32 system_mgmt_mode_get_num_smi_events(void);
const smi_event_t* system_mgmt_mode_get_smi_event(u32 index);
u64 system_mgmt_mode_get_total_smm_time(void);
u64 system_mgmt_mode_get_last_smi_time(void);
u32 system_mgmt_mode_get_smm_entries(void);
u32 system_mgmt_mode_get_smm_exits(void);
u32 system_mgmt_mode_get_smi_source_count(u8 smi_source);

/* Utilities */
const char* system_mgmt_mode_get_smi_source_name(u8 smi_source);
void system_mgmt_mode_clear_statistics(void);

#endif
