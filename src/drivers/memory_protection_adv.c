/*
 * memory_protection_adv.c – x86 advanced memory protection (SMEP/SMAP/PKU/CET)
 */

#include "kernel/types.h"

/* Page table entry bits for advanced protection */
#define PTE_PRESENT                     (1ULL << 0)
#define PTE_WRITE                       (1ULL << 1)
#define PTE_USER                        (1ULL << 2)
#define PTE_PWT                         (1ULL << 3)
#define PTE_PCD                         (1ULL << 4)
#define PTE_ACCESSED                    (1ULL << 5)
#define PTE_DIRTY                       (1ULL << 6)
#define PTE_PAT                         (1ULL << 7)
#define PTE_GLOBAL                      (1ULL << 8)
#define PTE_NX                          (1ULL << 63)

/* Protection Key bits in page table entries */
#define PTE_PKEY_MASK                   (0xFULL << 59)
#define PTE_PKEY_SHIFT                  59

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

typedef struct {
    u8 memory_protection_supported;
    u8 nx_supported;
    u8 smep_active;
    u8 smap_active;
    u8 pku_active;
    u8 cet_active;
    u32 num_protected_regions;
    protected_region_t protected_regions[256];
    u32 num_violations;
    protection_violation_t violations[128];
    u32 violation_index;
    u32 total_access_checks;
    u32 access_denied_count;
    u64 last_violation_time;
    u32 protection_keys_used;
    u8 domain_permissions[4][4];  /* [domain][access_type] */
} memory_protection_info_t;

static memory_protection_info_t mem_prot_info;

extern u64 timer_get_ticks(void);
extern u32 cpu_get_current_id(void);

static void memory_protection_detect_features(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check for NX bit support */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x80000001));
    mem_prot_info.nx_supported = (edx & (1 << 20)) != 0;
    
    /* Check for SMEP/SMAP support */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    
    u64 cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    
    mem_prot_info.smep_active = (cr4 & (1ULL << 20)) != 0;
    mem_prot_info.smap_active = (cr4 & (1ULL << 21)) != 0;
    mem_prot_info.pku_active = (cr4 & (1ULL << 22)) != 0;
    mem_prot_info.cet_active = (cr4 & (1ULL << 23)) != 0;
    
    if (mem_prot_info.nx_supported || mem_prot_info.smep_active || 
        mem_prot_info.smap_active || mem_prot_info.pku_active) {
        mem_prot_info.memory_protection_supported = 1;
    }
}

void memory_protection_adv_init(void) {
    mem_prot_info.memory_protection_supported = 0;
    mem_prot_info.nx_supported = 0;
    mem_prot_info.smep_active = 0;
    mem_prot_info.smap_active = 0;
    mem_prot_info.pku_active = 0;
    mem_prot_info.cet_active = 0;
    mem_prot_info.num_protected_regions = 0;
    mem_prot_info.num_violations = 0;
    mem_prot_info.violation_index = 0;
    mem_prot_info.total_access_checks = 0;
    mem_prot_info.access_denied_count = 0;
    mem_prot_info.last_violation_time = 0;
    mem_prot_info.protection_keys_used = 0;
    
    for (u32 i = 0; i < 256; i++) {
        mem_prot_info.protected_regions[i].virtual_address = 0;
        mem_prot_info.protected_regions[i].physical_address = 0;
        mem_prot_info.protected_regions[i].size = 0;
        mem_prot_info.protected_regions[i].protection_key = 0;
        mem_prot_info.protected_regions[i].access_permissions = 0;
        mem_prot_info.protected_regions[i].domain = 0;
        mem_prot_info.protected_regions[i].access_count = 0;
        mem_prot_info.protected_regions[i].last_access_time = 0;
    }
    
    for (u32 i = 0; i < 128; i++) {
        mem_prot_info.violations[i].fault_address = 0;
        mem_prot_info.violations[i].violation_type = 0;
        mem_prot_info.violations[i].access_type = 0;
        mem_prot_info.violations[i].protection_key = 0;
        mem_prot_info.violations[i].timestamp = 0;
        mem_prot_info.violations[i].cpu_id = 0;
        mem_prot_info.violations[i].instruction_pointer = 0;
    }
    
    /* Initialize domain permissions */
    for (u8 domain = 0; domain < 4; domain++) {
        for (u8 access = 0; access < 4; access++) {
            mem_prot_info.domain_permissions[domain][access] = 1;  /* Allow by default */
        }
    }
    
    /* Restrict some domain permissions */
    mem_prot_info.domain_permissions[PROTECTION_DOMAIN_USER][ACCESS_TYPE_EXECUTE] = 0;
    mem_prot_info.domain_permissions[PROTECTION_DOMAIN_SECURE][ACCESS_TYPE_WRITE] = 0;
    
    memory_protection_detect_features();
}

u8 memory_protection_adv_is_supported(void) {
    return mem_prot_info.memory_protection_supported;
}

u8 memory_protection_adv_register_region(u64 virtual_addr, u64 physical_addr, u64 size, 
                                         u8 protection_key, u8 permissions, u8 domain) {
    if (mem_prot_info.num_protected_regions >= 256) return 0;
    
    protected_region_t* region = &mem_prot_info.protected_regions[mem_prot_info.num_protected_regions];
    
    region->virtual_address = virtual_addr;
    region->physical_address = physical_addr;
    region->size = size;
    region->protection_key = protection_key;
    region->access_permissions = permissions;
    region->domain = domain;
    region->access_count = 0;
    region->last_access_time = 0;
    
    mem_prot_info.num_protected_regions++;
    
    if (protection_key > 0) {
        mem_prot_info.protection_keys_used |= (1 << protection_key);
    }
    
    return 1;
}

u8 memory_protection_adv_check_access(u64 virtual_addr, u8 access_type, u8 current_domain) {
    mem_prot_info.total_access_checks++;
    
    /* Find the protected region */
    protected_region_t* region = 0;
    for (u32 i = 0; i < mem_prot_info.num_protected_regions; i++) {
        if (virtual_addr >= mem_prot_info.protected_regions[i].virtual_address &&
            virtual_addr < (mem_prot_info.protected_regions[i].virtual_address + 
                           mem_prot_info.protected_regions[i].size)) {
            region = &mem_prot_info.protected_regions[i];
            break;
        }
    }
    
    if (!region) return 1;  /* No protection for this region */
    
    /* Check domain permissions */
    if (!mem_prot_info.domain_permissions[current_domain][access_type]) {
        mem_prot_info.access_denied_count++;
        return 0;
    }
    
    /* Check region-specific permissions */
    u8 required_perm = (1 << access_type);
    if (!(region->access_permissions & required_perm)) {
        mem_prot_info.access_denied_count++;
        return 0;
    }
    
    /* Update access statistics */
    region->access_count++;
    region->last_access_time = timer_get_ticks();
    
    return 1;
}

void memory_protection_adv_handle_violation(u64 fault_addr, u8 violation_type, 
                                           u8 access_type, u64 instruction_ptr) {
    protection_violation_t* violation = &mem_prot_info.violations[mem_prot_info.violation_index];
    
    violation->fault_address = fault_addr;
    violation->violation_type = violation_type;
    violation->access_type = access_type;
    violation->timestamp = timer_get_ticks();
    violation->cpu_id = cpu_get_current_id();
    violation->instruction_pointer = instruction_ptr;
    
    /* Find protection key if applicable */
    violation->protection_key = 0;
    for (u32 i = 0; i < mem_prot_info.num_protected_regions; i++) {
        if (fault_addr >= mem_prot_info.protected_regions[i].virtual_address &&
            fault_addr < (mem_prot_info.protected_regions[i].virtual_address + 
                         mem_prot_info.protected_regions[i].size)) {
            violation->protection_key = mem_prot_info.protected_regions[i].protection_key;
            break;
        }
    }
    
    mem_prot_info.violation_index = (mem_prot_info.violation_index + 1) % 128;
    if (mem_prot_info.num_violations < 128) {
        mem_prot_info.num_violations++;
    }
    
    mem_prot_info.last_violation_time = violation->timestamp;
}

u64 memory_protection_adv_set_page_protection_key(u64 page_addr, u8 protection_key) {
    if (!mem_prot_info.pku_active || protection_key >= 16) return 0;
    
    /* Get page table entry address (simplified) */
    u64* pte = (u64*)(0xFFFF800000000000ULL + ((page_addr >> 12) * 8));
    
    /* Clear existing protection key bits */
    *pte &= ~PTE_PKEY_MASK;
    
    /* Set new protection key */
    *pte |= ((u64)protection_key << PTE_PKEY_SHIFT);
    
    /* Invalidate TLB for this page */
    __asm__ volatile("invlpg (%0)" : : "r"(page_addr) : "memory");
    
    return *pte;
}

u8 memory_protection_adv_set_nx_bit(u64 page_addr, u8 enable) {
    if (!mem_prot_info.nx_supported) return 0;
    
    /* Get page table entry address (simplified) */
    u64* pte = (u64*)(0xFFFF800000000000ULL + ((page_addr >> 12) * 8));
    
    if (enable) {
        *pte |= PTE_NX;
    } else {
        *pte &= ~PTE_NX;
    }
    
    /* Invalidate TLB for this page */
    __asm__ volatile("invlpg (%0)" : : "r"(page_addr) : "memory");
    
    return 1;
}

void memory_protection_adv_set_domain_permission(u8 domain, u8 access_type, u8 allowed) {
    if (domain >= 4 || access_type >= 4) return;
    
    mem_prot_info.domain_permissions[domain][access_type] = allowed;
}

u8 memory_protection_adv_get_domain_permission(u8 domain, u8 access_type) {
    if (domain >= 4 || access_type >= 4) return 0;
    
    return mem_prot_info.domain_permissions[domain][access_type];
}

u8 memory_protection_adv_is_nx_supported(void) {
    return mem_prot_info.nx_supported;
}

u8 memory_protection_adv_is_smep_active(void) {
    return mem_prot_info.smep_active;
}

u8 memory_protection_adv_is_smap_active(void) {
    return mem_prot_info.smap_active;
}

u8 memory_protection_adv_is_pku_active(void) {
    return mem_prot_info.pku_active;
}

u8 memory_protection_adv_is_cet_active(void) {
    return mem_prot_info.cet_active;
}

u32 memory_protection_adv_get_num_protected_regions(void) {
    return mem_prot_info.num_protected_regions;
}

const protected_region_t* memory_protection_adv_get_region_info(u32 index) {
    if (index >= mem_prot_info.num_protected_regions) return 0;
    return &mem_prot_info.protected_regions[index];
}

u32 memory_protection_adv_get_num_violations(void) {
    return mem_prot_info.num_violations;
}

const protection_violation_t* memory_protection_adv_get_violation_info(u32 index) {
    if (index >= mem_prot_info.num_violations) return 0;
    return &mem_prot_info.violations[index];
}

u32 memory_protection_adv_get_total_access_checks(void) {
    return mem_prot_info.total_access_checks;
}

u32 memory_protection_adv_get_access_denied_count(void) {
    return mem_prot_info.access_denied_count;
}

u64 memory_protection_adv_get_last_violation_time(void) {
    return mem_prot_info.last_violation_time;
}

u32 memory_protection_adv_get_protection_keys_used(void) {
    return mem_prot_info.protection_keys_used;
}

const char* memory_protection_adv_get_violation_type_name(u8 violation_type) {
    switch (violation_type) {
        case VIOLATION_TYPE_SMEP: return "SMEP";
        case VIOLATION_TYPE_SMAP: return "SMAP";
        case VIOLATION_TYPE_PKEY: return "Protection Key";
        case VIOLATION_TYPE_NX: return "NX Bit";
        case VIOLATION_TYPE_CET: return "CET";
        default: return "Unknown";
    }
}

const char* memory_protection_adv_get_access_type_name(u8 access_type) {
    switch (access_type) {
        case ACCESS_TYPE_READ: return "Read";
        case ACCESS_TYPE_WRITE: return "Write";
        case ACCESS_TYPE_EXECUTE: return "Execute";
        default: return "Unknown";
    }
}

const char* memory_protection_adv_get_domain_name(u8 domain) {
    switch (domain) {
        case PROTECTION_DOMAIN_KERNEL: return "Kernel";
        case PROTECTION_DOMAIN_USER: return "User";
        case PROTECTION_DOMAIN_DRIVER: return "Driver";
        case PROTECTION_DOMAIN_SECURE: return "Secure";
        default: return "Unknown";
    }
}

void memory_protection_adv_clear_statistics(void) {
    mem_prot_info.total_access_checks = 0;
    mem_prot_info.access_denied_count = 0;
    mem_prot_info.num_violations = 0;
    mem_prot_info.violation_index = 0;
    
    for (u32 i = 0; i < mem_prot_info.num_protected_regions; i++) {
        mem_prot_info.protected_regions[i].access_count = 0;
    }
}
