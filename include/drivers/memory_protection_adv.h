/*
 * memory_protection_adv.h – x86 advanced memory protection (SMEP/SMAP/PKU/CET)
 */

#ifndef MEMORY_PROTECTION_ADV_H
#define MEMORY_PROTECTION_ADV_H

#include "kernel/types.h"

/* Memory protection domains */
#define PROTECTION_DOMAIN_KERNEL        0
#define PROTECTION_DOMAIN_USER          1
#define PROTECTION_DOMAIN_DRIVER        2
#define PROTECTION_DOMAIN_SECURE        3

/* Memory access types */
#define ACCESS_TYPE_READ                0
#define ACCESS_TYPE_WRITE               1
#define ACCESS_TYPE_EXECUTE             2

/* Protection violation types */
#define VIOLATION_TYPE_SMEP             0
#define VIOLATION_TYPE_SMAP             1
#define VIOLATION_TYPE_PKEY             2
#define VIOLATION_TYPE_NX               3
#define VIOLATION_TYPE_CET              4

typedef struct {
    u64 virtual_address;
    u64 physical_address;
    u64 size;
    u8 protection_key;
    u8 access_permissions;
    u8 domain;
    u32 access_count;
    u64 last_access_time;
} protected_region_t;

typedef struct {
    u64 fault_address;
    u8 violation_type;
    u8 access_type;
    u8 protection_key;
    u64 timestamp;
    u32 cpu_id;
    u64 instruction_pointer;
} protection_violation_t;

/* Core functions */
void memory_protection_adv_init(void);

/* Support detection */
u8 memory_protection_adv_is_supported(void);
u8 memory_protection_adv_is_nx_supported(void);
u8 memory_protection_adv_is_smep_active(void);
u8 memory_protection_adv_is_smap_active(void);
u8 memory_protection_adv_is_pku_active(void);
u8 memory_protection_adv_is_cet_active(void);

/* Region management */
u8 memory_protection_adv_register_region(u64 virtual_addr, u64 physical_addr, u64 size, 
                                         u8 protection_key, u8 permissions, u8 domain);
u32 memory_protection_adv_get_num_protected_regions(void);
const protected_region_t* memory_protection_adv_get_region_info(u32 index);

/* Access control */
u8 memory_protection_adv_check_access(u64 virtual_addr, u8 access_type, u8 current_domain);
void memory_protection_adv_set_domain_permission(u8 domain, u8 access_type, u8 allowed);
u8 memory_protection_adv_get_domain_permission(u8 domain, u8 access_type);

/* Page-level protection */
u64 memory_protection_adv_set_page_protection_key(u64 page_addr, u8 protection_key);
u8 memory_protection_adv_set_nx_bit(u64 page_addr, u8 enable);

/* Violation handling */
void memory_protection_adv_handle_violation(u64 fault_addr, u8 violation_type, 
                                           u8 access_type, u64 instruction_ptr);
u32 memory_protection_adv_get_num_violations(void);
const protection_violation_t* memory_protection_adv_get_violation_info(u32 index);

/* Statistics */
u32 memory_protection_adv_get_total_access_checks(void);
u32 memory_protection_adv_get_access_denied_count(void);
u64 memory_protection_adv_get_last_violation_time(void);
u32 memory_protection_adv_get_protection_keys_used(void);

/* Utilities */
const char* memory_protection_adv_get_violation_type_name(u8 violation_type);
const char* memory_protection_adv_get_access_type_name(u8 access_type);
const char* memory_protection_adv_get_domain_name(u8 domain);
void memory_protection_adv_clear_statistics(void);

#endif
