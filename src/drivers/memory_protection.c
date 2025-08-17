/*
 * memory_protection.c – x86 advanced memory protection (SMEP/SMAP extensions)
 */

#include "kernel/types.h"

/* Control register bits */
#define CR4_SMEP                (1 << 20)
#define CR4_SMAP                (1 << 21)
#define CR4_PKE                 (1 << 22)
#define CR4_CET                 (1 << 23)

/* EFLAGS bits */
#define EFLAGS_AC               (1 << 18)

/* Protection key rights */
#define PKEY_DISABLE_ACCESS     0x1
#define PKEY_DISABLE_WRITE      0x2

/* CET MSRs */
#define MSR_U_CET               0x6A0
#define MSR_S_CET               0x6A2
#define MSR_PL0_SSP             0x6A4
#define MSR_PL1_SSP             0x6A5
#define MSR_PL2_SSP             0x6A6
#define MSR_PL3_SSP             0x6A7

/* CET control bits */
#define CET_SH_STK_EN           (1 << 0)
#define CET_WR_SHSTK_EN         (1 << 1)
#define CET_ENDBR_EN            (1 << 2)
#define CET_LEG_IW_EN           (1 << 3)
#define CET_NO_TRACK_EN         (1 << 4)
#define CET_SUPPRESS_DIS        (1 << 5)
#define CET_SUPPRESS            (1 << 10)
#define CET_TRACKER             (1 << 11)

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

typedef struct {
    u8 memory_protection_supported;
    u8 smep_supported;
    u8 smap_supported;
    u8 pku_supported;
    u8 cet_supported;
    u8 smep_enabled;
    u8 smap_enabled;
    u8 pku_enabled;
    u8 cet_enabled;
    u8 shadow_stack_enabled;
    u8 indirect_branch_tracking_enabled;
    u32 pkru_value;
    u64 shadow_stack_pointer;
    u32 violation_count;
    protection_violation_t violations[32];
    u8 protection_keys_allocated[16];
} memory_protection_info_t;

static memory_protection_info_t memory_protection_info;

extern u64 msr_read(u32 msr);
extern void msr_write(u32 msr, u64 value);
extern u8 msr_is_supported(void);

static void memory_protection_detect_capabilities(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check for SMEP/SMAP support */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    
    memory_protection_info.smep_supported = (ebx & (1 << 7)) != 0;
    memory_protection_info.smap_supported = (ebx & (1 << 20)) != 0;
    memory_protection_info.pku_supported = (ecx & (1 << 3)) != 0;
    
    /* Check for CET support */
    memory_protection_info.cet_supported = (ecx & (1 << 7)) != 0;
    
    if (memory_protection_info.smep_supported || memory_protection_info.smap_supported ||
        memory_protection_info.pku_supported || memory_protection_info.cet_supported) {
        memory_protection_info.memory_protection_supported = 1;
    }
}

static void memory_protection_enable_smep(void) {
    if (!memory_protection_info.smep_supported) return;
    
    u32 cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= CR4_SMEP;
    __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));
    
    memory_protection_info.smep_enabled = 1;
}

static void memory_protection_enable_smap(void) {
    if (!memory_protection_info.smap_supported) return;
    
    u32 cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= CR4_SMAP;
    __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));
    
    memory_protection_info.smap_enabled = 1;
}

static void memory_protection_enable_pku(void) {
    if (!memory_protection_info.pku_supported) return;
    
    u32 cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= CR4_PKE;
    __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));
    
    /* Initialize PKRU to allow all access */
    __asm__ volatile("xor %%eax, %%eax; xor %%ecx, %%ecx; xor %%edx, %%edx; wrpkru" : : : "eax", "ecx", "edx");
    memory_protection_info.pkru_value = 0;
    memory_protection_info.pku_enabled = 1;
}

static void memory_protection_enable_cet(void) {
    if (!memory_protection_info.cet_supported || !msr_is_supported()) return;
    
    u32 cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= CR4_CET;
    __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));
    
    /* Enable shadow stack for supervisor mode */
    u64 s_cet = CET_SH_STK_EN | CET_WR_SHSTK_EN;
    msr_write(MSR_S_CET, s_cet);
    
    /* Enable indirect branch tracking */
    s_cet |= CET_ENDBR_EN;
    msr_write(MSR_S_CET, s_cet);
    
    memory_protection_info.cet_enabled = 1;
    memory_protection_info.shadow_stack_enabled = 1;
    memory_protection_info.indirect_branch_tracking_enabled = 1;
}

void memory_protection_init(void) {
    memory_protection_info.memory_protection_supported = 0;
    memory_protection_info.smep_supported = 0;
    memory_protection_info.smap_supported = 0;
    memory_protection_info.pku_supported = 0;
    memory_protection_info.cet_supported = 0;
    memory_protection_info.smep_enabled = 0;
    memory_protection_info.smap_enabled = 0;
    memory_protection_info.pku_enabled = 0;
    memory_protection_info.cet_enabled = 0;
    memory_protection_info.shadow_stack_enabled = 0;
    memory_protection_info.indirect_branch_tracking_enabled = 0;
    memory_protection_info.pkru_value = 0;
    memory_protection_info.shadow_stack_pointer = 0;
    memory_protection_info.violation_count = 0;
    
    for (u8 i = 0; i < 16; i++) {
        memory_protection_info.protection_keys_allocated[i] = 0;
    }
    
    memory_protection_detect_capabilities();
    
    if (memory_protection_info.memory_protection_supported) {
        memory_protection_enable_smep();
        memory_protection_enable_smap();
        memory_protection_enable_pku();
        memory_protection_enable_cet();
    }
}

u8 memory_protection_is_supported(void) {
    return memory_protection_info.memory_protection_supported;
}

u8 memory_protection_is_smep_supported(void) {
    return memory_protection_info.smep_supported;
}

u8 memory_protection_is_smap_supported(void) {
    return memory_protection_info.smap_supported;
}

u8 memory_protection_is_pku_supported(void) {
    return memory_protection_info.pku_supported;
}

u8 memory_protection_is_cet_supported(void) {
    return memory_protection_info.cet_supported;
}

u8 memory_protection_is_smep_enabled(void) {
    return memory_protection_info.smep_enabled;
}

u8 memory_protection_is_smap_enabled(void) {
    return memory_protection_info.smap_enabled;
}

u8 memory_protection_is_pku_enabled(void) {
    return memory_protection_info.pku_enabled;
}

u8 memory_protection_is_cet_enabled(void) {
    return memory_protection_info.cet_enabled;
}

void memory_protection_stac(void) {
    if (!memory_protection_info.smap_enabled) return;
    
    __asm__ volatile("stac" : : : "cc");
}

void memory_protection_clac(void) {
    if (!memory_protection_info.smap_enabled) return;
    
    __asm__ volatile("clac" : : : "cc");
}

u8 memory_protection_allocate_protection_key(void) {
    if (!memory_protection_info.pku_enabled) return 0xFF;
    
    for (u8 i = 1; i < 16; i++) { /* Key 0 is reserved */
        if (!memory_protection_info.protection_keys_allocated[i]) {
            memory_protection_info.protection_keys_allocated[i] = 1;
            return i;
        }
    }
    
    return 0xFF; /* No keys available */
}

void memory_protection_free_protection_key(u8 key) {
    if (!memory_protection_info.pku_enabled || key == 0 || key >= 16) return;
    
    memory_protection_info.protection_keys_allocated[key] = 0;
    
    /* Clear permissions for this key */
    u32 pkru = memory_protection_info.pkru_value;
    pkru &= ~(3 << (key * 2)); /* Clear both access and write disable bits */
    memory_protection_set_pkru(pkru);
}

void memory_protection_set_protection_key_rights(u8 key, u8 access_disable, u8 write_disable) {
    if (!memory_protection_info.pku_enabled || key >= 16) return;
    
    u32 pkru = memory_protection_info.pkru_value;
    u32 key_bits = (key * 2);
    
    pkru &= ~(3 << key_bits); /* Clear existing bits */
    
    if (access_disable) pkru |= (PKEY_DISABLE_ACCESS << key_bits);
    if (write_disable) pkru |= (PKEY_DISABLE_WRITE << key_bits);
    
    memory_protection_set_pkru(pkru);
}

u32 memory_protection_get_pkru(void) {
    if (!memory_protection_info.pku_enabled) return 0;
    
    u32 pkru;
    __asm__ volatile("rdpkru" : "=a"(pkru) : "c"(0) : "edx");
    return pkru;
}

void memory_protection_set_pkru(u32 pkru) {
    if (!memory_protection_info.pku_enabled) return;
    
    __asm__ volatile("wrpkru" : : "a"(pkru), "c"(0), "d"(0));
    memory_protection_info.pkru_value = pkru;
}

void memory_protection_setup_shadow_stack(u64 stack_base, u32 stack_size) {
    if (!memory_protection_info.cet_enabled || !msr_is_supported()) return;
    
    /* Setup supervisor shadow stack */
    msr_write(MSR_PL0_SSP, stack_base + stack_size);
    memory_protection_info.shadow_stack_pointer = stack_base + stack_size;
}

u64 memory_protection_get_shadow_stack_pointer(void) {
    if (!memory_protection_info.cet_enabled || !msr_is_supported()) return 0;
    
    return msr_read(MSR_PL0_SSP);
}

void memory_protection_handle_violation(u32 violation_type, u64 fault_address, u32 error_code, u64 instruction_pointer) {
    if (memory_protection_info.violation_count >= 32) return;
    
    protection_violation_t* violation = &memory_protection_info.violations[memory_protection_info.violation_count];
    
    violation->violation_type = violation_type;
    violation->fault_address = fault_address;
    violation->error_code = error_code;
    violation->instruction_pointer = instruction_pointer;
    violation->protection_key = 0;
    
    if (violation_type == VIOLATION_PKU) {
        violation->protection_key = memory_protection_get_pkru();
    }
    
    extern u64 timer_get_ticks(void);
    violation->timestamp = timer_get_ticks();
    
    memory_protection_info.violation_count++;
}

u32 memory_protection_get_violation_count(void) {
    return memory_protection_info.violation_count;
}

const protection_violation_t* memory_protection_get_violation(u32 index) {
    if (index >= memory_protection_info.violation_count) return 0;
    return &memory_protection_info.violations[index];
}

void memory_protection_clear_violations(void) {
    memory_protection_info.violation_count = 0;
}

u8 memory_protection_validate_user_pointer(const void* ptr, u32 size) {
    if (!memory_protection_info.smep_enabled && !memory_protection_info.smap_enabled) return 1;
    
    u64 addr = (u64)ptr;
    
    /* Check if address is in user space (simplified check) */
    if (addr >= 0x80000000) return 0; /* Kernel space */
    
    /* Additional validation could be added here */
    return 1;
}

void memory_protection_enable_supervisor_access(void) {
    if (!memory_protection_info.smap_enabled) return;
    
    memory_protection_stac();
}

void memory_protection_disable_supervisor_access(void) {
    if (!memory_protection_info.smap_enabled) return;
    
    memory_protection_clac();
}

u8 memory_protection_is_supervisor_access_enabled(void) {
    if (!memory_protection_info.smap_enabled) return 1;
    
    u32 eflags;
    __asm__ volatile("pushf; pop %0" : "=r"(eflags));
    return (eflags & EFLAGS_AC) != 0;
}

void memory_protection_disable_smep(void) {
    if (!memory_protection_info.smep_enabled) return;
    
    u32 cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 &= ~CR4_SMEP;
    __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));
    
    memory_protection_info.smep_enabled = 0;
}

void memory_protection_disable_smap(void) {
    if (!memory_protection_info.smap_enabled) return;
    
    u32 cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 &= ~CR4_SMAP;
    __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));
    
    memory_protection_info.smap_enabled = 0;
}

u8 memory_protection_is_shadow_stack_enabled(void) {
    return memory_protection_info.shadow_stack_enabled;
}

u8 memory_protection_is_indirect_branch_tracking_enabled(void) {
    return memory_protection_info.indirect_branch_tracking_enabled;
}

void memory_protection_endbr64(void) {
    if (!memory_protection_info.indirect_branch_tracking_enabled) return;
    
    __asm__ volatile("endbr64");
}

void memory_protection_endbr32(void) {
    if (!memory_protection_info.indirect_branch_tracking_enabled) return;
    
    __asm__ volatile("endbr32");
}
