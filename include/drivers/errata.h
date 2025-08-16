/*
 * errata.h – x86 CPU errata detection and workarounds
 */

#ifndef ERRATA_H
#define ERRATA_H

#include "kernel/types.h"

/* Errata types */
#define ERRATA_TYPE_PERFORMANCE 0x01
#define ERRATA_TYPE_SECURITY    0x02
#define ERRATA_TYPE_FUNCTIONAL  0x04
#define ERRATA_TYPE_CRITICAL    0x08

/* Core functions */
void errata_init(void);

/* CPU information */
u32 errata_get_cpu_signature(void);
u32 errata_get_family(void);
u32 errata_get_model(void);
u32 errata_get_stepping(void);
u8 errata_is_intel(void);
u8 errata_is_amd(void);

/* Errata information */
u32 errata_get_active_count(void);
u16 errata_get_active_id(u32 index);
const char* errata_get_description(u16 errata_id);
u8 errata_get_type(u16 errata_id);
u8 errata_get_severity(u16 errata_id);

/* Errata classification */
u8 errata_is_security_related(u16 errata_id);
u8 errata_is_performance_related(u16 errata_id);
u8 errata_is_functional_related(u16 errata_id);
u8 errata_is_critical(u16 errata_id);

/* Workaround management */
void errata_apply_workaround(u16 errata_id);
u8 errata_check_vulnerability(const char* vuln_name);

#endif
