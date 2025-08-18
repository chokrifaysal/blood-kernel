/*
 * cpu_errata.h – x86 CPU errata and microcode management
 */

#ifndef CPU_ERRATA_H
#define CPU_ERRATA_H

#include "kernel/types.h"

/* Known CPU errata */
typedef struct {
    u32 cpu_signature;
    u32 errata_id;
    const char* description;
    u8 severity;  /* 0=low, 1=medium, 2=high, 3=critical */
    u8 workaround_available;
    u8 microcode_fixes;
} cpu_errata_t;

/* Core functions */
void cpu_errata_init(void);

/* Support detection */
u8 cpu_errata_is_supported(void);
u8 cpu_errata_is_microcode_supported(void);
u8 cpu_errata_is_intel(void);
u8 cpu_errata_is_amd(void);

/* CPU information */
u32 cpu_errata_get_cpu_signature(void);
u32 cpu_errata_get_platform_id(void);

/* Microcode management */
u32 cpu_errata_get_microcode_revision(void);
u32 cpu_errata_get_bios_microcode_revision(void);
u8 cpu_errata_load_microcode(const void* microcode_data, u32 size);
u8 cpu_errata_is_microcode_loaded(void);
u32 cpu_errata_get_microcode_date(void);
void cpu_errata_refresh_microcode_revision(void);
u8 cpu_errata_needs_microcode_update(void);

/* Errata detection and management */
u32 cpu_errata_get_num_errata_found(void);
const cpu_errata_t* cpu_errata_get_errata_info(u32 index);
u8 cpu_errata_apply_workaround(u32 errata_id);
u8 cpu_errata_check_vulnerability(const char* vulnerability_name);
u8 cpu_errata_has_critical_errata(void);
u32 cpu_errata_count_by_severity(u8 severity);

/* Statistics */
u32 cpu_errata_get_workarounds_applied(void);
u32 cpu_errata_get_microcode_updates_applied(void);
u64 cpu_errata_get_last_microcode_update_time(void);

/* Utilities */
const char* cpu_errata_get_severity_name(u8 severity);
void cpu_errata_clear_statistics(void);

#endif
