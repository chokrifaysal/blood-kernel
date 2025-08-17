/*
 * memory_adv.c – x86 advanced memory management (PCID/INVPCID)
 */

#include "kernel/types.h"

/* CR4 control bits */
#define CR4_PCIDE                   (1 << 17)

/* INVPCID types */
#define INVPCID_TYPE_INDIVIDUAL     0
#define INVPCID_TYPE_SINGLE_CONTEXT 1
#define INVPCID_TYPE_ALL_GLOBAL     2
#define INVPCID_TYPE_ALL_NON_GLOBAL 3

/* PCID management */
#define MAX_PCID                    4096
#define PCID_KERNEL                 0
#define PCID_USER_BASE              1

/* Memory types */
#define MEMORY_TYPE_UC              0x00
#define MEMORY_TYPE_WC              0x01
#define MEMORY_TYPE_WT              0x04
#define MEMORY_TYPE_WP              0x05
#define MEMORY_TYPE_WB              0x06

/* Page attribute table */
#define PAT_UC                      0x00
#define PAT_WC                      0x01
#define PAT_WT                      0x04
#define PAT_WP                      0x05
#define PAT_WB                      0x06
#define PAT_UC_MINUS                0x07

typedef struct {
    u16 pcid;
    u64 cr3_base;
    u8 active;
    u8 global;
    u32 tlb_flushes;
    u64 last_used;
} pcid_entry_t;

typedef struct {
    u64 base_address;
    u64 size;
    u8 memory_type;
    u8 enabled;
} mtrr_entry_t;

typedef struct {
    u8 memory_adv_supported;
    u8 pcid_supported;
    u8 invpcid_supported;
    u8 pcid_enabled;
    u8 pat_supported;
    u8 mtrr_supported;
    u16 current_pcid;
    u16 next_pcid;
    pcid_entry_t pcid_table[64];
    u8 num_mtrr_entries;
    mtrr_entry_t mtrr_entries[16];
    u64 pat_value;
    u32 total_tlb_flushes;
    u32 pcid_switches;
    u32 invpcid_calls;
} memory_adv_info_t;

static memory_adv_info_t memory_adv_info;

extern u64 msr_read(u32 msr);
extern void msr_write(u32 msr, u64 value);
extern u8 msr_is_supported(void);
extern u64 timer_get_ticks(void);

static void memory_adv_detect_capabilities(void) {
    u32 eax, ebx, ecx, edx;
    
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    memory_adv_info.pat_supported = (edx & (1 << 16)) != 0;
    memory_adv_info.mtrr_supported = (edx & (1 << 12)) != 0;
    
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    memory_adv_info.pcid_supported = (ecx & (1 << 17)) != 0;
    memory_adv_info.invpcid_supported = (ebx & (1 << 10)) != 0;
    
    if (memory_adv_info.pcid_supported || memory_adv_info.pat_supported || memory_adv_info.mtrr_supported) {
        memory_adv_info.memory_adv_supported = 1;
    }
}

static void memory_adv_setup_pat(void) {
    if (!memory_adv_info.pat_supported || !msr_is_supported()) return;
    
    u64 pat = 0;
    pat |= ((u64)PAT_WB << 0);
    pat |= ((u64)PAT_WT << 8);
    pat |= ((u64)PAT_UC_MINUS << 16);
    pat |= ((u64)PAT_UC << 24);
    pat |= ((u64)PAT_WB << 32);
    pat |= ((u64)PAT_WT << 40);
    pat |= ((u64)PAT_WC << 48);
    pat |= ((u64)PAT_UC << 56);
    
    msr_write(0x277, pat);
    memory_adv_info.pat_value = pat;
}

static void memory_adv_enable_pcid(void) {
    if (!memory_adv_info.pcid_supported) return;
    
    u64 cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= CR4_PCIDE;
    __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));
    
    memory_adv_info.pcid_enabled = 1;
    memory_adv_info.current_pcid = PCID_KERNEL;
    memory_adv_info.next_pcid = PCID_USER_BASE;
}

void memory_adv_init(void) {
    memory_adv_info.memory_adv_supported = 0;
    memory_adv_info.pcid_supported = 0;
    memory_adv_info.invpcid_supported = 0;
    memory_adv_info.pcid_enabled = 0;
    memory_adv_info.pat_supported = 0;
    memory_adv_info.mtrr_supported = 0;
    memory_adv_info.current_pcid = 0;
    memory_adv_info.next_pcid = 1;
    memory_adv_info.num_mtrr_entries = 0;
    memory_adv_info.pat_value = 0;
    memory_adv_info.total_tlb_flushes = 0;
    memory_adv_info.pcid_switches = 0;
    memory_adv_info.invpcid_calls = 0;
    
    for (u8 i = 0; i < 64; i++) {
        memory_adv_info.pcid_table[i].pcid = 0;
        memory_adv_info.pcid_table[i].cr3_base = 0;
        memory_adv_info.pcid_table[i].active = 0;
        memory_adv_info.pcid_table[i].global = 0;
        memory_adv_info.pcid_table[i].tlb_flushes = 0;
        memory_adv_info.pcid_table[i].last_used = 0;
    }
    
    for (u8 i = 0; i < 16; i++) {
        memory_adv_info.mtrr_entries[i].base_address = 0;
        memory_adv_info.mtrr_entries[i].size = 0;
        memory_adv_info.mtrr_entries[i].memory_type = MEMORY_TYPE_UC;
        memory_adv_info.mtrr_entries[i].enabled = 0;
    }
    
    memory_adv_detect_capabilities();
    
    if (memory_adv_info.memory_adv_supported) {
        memory_adv_setup_pat();
        memory_adv_enable_pcid();
    }
}

u8 memory_adv_is_supported(void) {
    return memory_adv_info.memory_adv_supported;
}

u8 memory_adv_is_pcid_supported(void) {
    return memory_adv_info.pcid_supported;
}

u8 memory_adv_is_invpcid_supported(void) {
    return memory_adv_info.invpcid_supported;
}

u8 memory_adv_is_pcid_enabled(void) {
    return memory_adv_info.pcid_enabled;
}

u16 memory_adv_allocate_pcid(u64 cr3_base) {
    if (!memory_adv_info.pcid_enabled) return 0;
    
    for (u8 i = 0; i < 64; i++) {
        if (!memory_adv_info.pcid_table[i].active) {
            memory_adv_info.pcid_table[i].pcid = memory_adv_info.next_pcid;
            memory_adv_info.pcid_table[i].cr3_base = cr3_base;
            memory_adv_info.pcid_table[i].active = 1;
            memory_adv_info.pcid_table[i].global = 0;
            memory_adv_info.pcid_table[i].tlb_flushes = 0;
            memory_adv_info.pcid_table[i].last_used = timer_get_ticks();
            
            u16 pcid = memory_adv_info.next_pcid;
            memory_adv_info.next_pcid++;
            if (memory_adv_info.next_pcid >= MAX_PCID) {
                memory_adv_info.next_pcid = PCID_USER_BASE;
            }
            
            return pcid;
        }
    }
    
    return 0;
}

void memory_adv_free_pcid(u16 pcid) {
    if (!memory_adv_info.pcid_enabled) return;
    
    for (u8 i = 0; i < 64; i++) {
        if (memory_adv_info.pcid_table[i].pcid == pcid && memory_adv_info.pcid_table[i].active) {
            memory_adv_info.pcid_table[i].active = 0;
            memory_adv_info.pcid_table[i].pcid = 0;
            memory_adv_info.pcid_table[i].cr3_base = 0;
            break;
        }
    }
}

void memory_adv_switch_pcid(u16 pcid, u64 cr3_base) {
    if (!memory_adv_info.pcid_enabled) return;
    
    u64 cr3_with_pcid = cr3_base | pcid;
    __asm__ volatile("mov %0, %%cr3" : : "r"(cr3_with_pcid));
    
    memory_adv_info.current_pcid = pcid;
    memory_adv_info.pcid_switches++;
    
    for (u8 i = 0; i < 64; i++) {
        if (memory_adv_info.pcid_table[i].pcid == pcid) {
            memory_adv_info.pcid_table[i].last_used = timer_get_ticks();
            break;
        }
    }
}

u16 memory_adv_get_current_pcid(void) {
    return memory_adv_info.current_pcid;
}

void memory_adv_invpcid(u32 type, u16 pcid, u64 linear_address) {
    if (!memory_adv_info.invpcid_supported) return;
    
    struct {
        u64 pcid_la;
        u64 reserved;
    } descriptor;
    
    descriptor.pcid_la = ((u64)pcid << 0) | (linear_address & ~0xFFF);
    descriptor.reserved = 0;
    
    __asm__ volatile("invpcid %0, %1" : : "m"(descriptor), "r"((u64)type));
    
    memory_adv_info.invpcid_calls++;
    
    if (type == INVPCID_TYPE_SINGLE_CONTEXT || type == INVPCID_TYPE_ALL_NON_GLOBAL) {
        for (u8 i = 0; i < 64; i++) {
            if (memory_adv_info.pcid_table[i].pcid == pcid || type == INVPCID_TYPE_ALL_NON_GLOBAL) {
                memory_adv_info.pcid_table[i].tlb_flushes++;
            }
        }
    }
    
    memory_adv_info.total_tlb_flushes++;
}

void memory_adv_flush_tlb_pcid(u16 pcid) {
    if (memory_adv_info.invpcid_supported) {
        memory_adv_invpcid(INVPCID_TYPE_SINGLE_CONTEXT, pcid, 0);
    } else if (memory_adv_info.pcid_enabled) {
        u64 cr3;
        __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
        cr3 = (cr3 & ~0xFFF) | pcid;
        __asm__ volatile("mov %0, %%cr3" : : "r"(cr3));
        memory_adv_info.total_tlb_flushes++;
    }
}

void memory_adv_flush_tlb_all(void) {
    if (memory_adv_info.invpcid_supported) {
        memory_adv_invpcid(INVPCID_TYPE_ALL_NON_GLOBAL, 0, 0);
    } else {
        u64 cr3;
        __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
        __asm__ volatile("mov %0, %%cr3" : : "r"(cr3));
        memory_adv_info.total_tlb_flushes++;
    }
}

u8 memory_adv_is_pat_supported(void) {
    return memory_adv_info.pat_supported;
}

u64 memory_adv_get_pat_value(void) {
    return memory_adv_info.pat_value;
}

void memory_adv_set_pat_entry(u8 index, u8 memory_type) {
    if (!memory_adv_info.pat_supported || !msr_is_supported() || index >= 8) return;
    
    u64 pat = memory_adv_info.pat_value;
    u64 mask = ~(0xFFULL << (index * 8));
    pat &= mask;
    pat |= ((u64)memory_type << (index * 8));
    
    msr_write(0x277, pat);
    memory_adv_info.pat_value = pat;
}

u8 memory_adv_get_pat_entry(u8 index) {
    if (index >= 8) return 0;
    return (memory_adv_info.pat_value >> (index * 8)) & 0xFF;
}

u8 memory_adv_is_mtrr_supported(void) {
    return memory_adv_info.mtrr_supported;
}

u8 memory_adv_setup_mtrr(u8 index, u64 base_address, u64 size, u8 memory_type) {
    if (!memory_adv_info.mtrr_supported || !msr_is_supported() || index >= 16) return 0;
    
    if (memory_adv_info.num_mtrr_entries >= 16) return 0;
    
    memory_adv_info.mtrr_entries[index].base_address = base_address;
    memory_adv_info.mtrr_entries[index].size = size;
    memory_adv_info.mtrr_entries[index].memory_type = memory_type;
    memory_adv_info.mtrr_entries[index].enabled = 1;
    
    u64 mask = ~(size - 1) | (1ULL << 11);
    u64 base = base_address | memory_type;
    
    msr_write(0x200 + (index * 2), base);
    msr_write(0x201 + (index * 2), mask);
    
    if (index >= memory_adv_info.num_mtrr_entries) {
        memory_adv_info.num_mtrr_entries = index + 1;
    }
    
    return 1;
}

void memory_adv_disable_mtrr(u8 index) {
    if (!memory_adv_info.mtrr_supported || !msr_is_supported() || index >= 16) return;
    
    memory_adv_info.mtrr_entries[index].enabled = 0;
    
    msr_write(0x201 + (index * 2), 0);
}

const mtrr_entry_t* memory_adv_get_mtrr_entry(u8 index) {
    if (index >= 16) return 0;
    return &memory_adv_info.mtrr_entries[index];
}

u8 memory_adv_get_num_mtrr_entries(void) {
    return memory_adv_info.num_mtrr_entries;
}

const pcid_entry_t* memory_adv_get_pcid_entry(u8 index) {
    if (index >= 64) return 0;
    return &memory_adv_info.pcid_table[index];
}

u32 memory_adv_get_total_tlb_flushes(void) {
    return memory_adv_info.total_tlb_flushes;
}

u32 memory_adv_get_pcid_switches(void) {
    return memory_adv_info.pcid_switches;
}

u32 memory_adv_get_invpcid_calls(void) {
    return memory_adv_info.invpcid_calls;
}

void memory_adv_clear_statistics(void) {
    memory_adv_info.total_tlb_flushes = 0;
    memory_adv_info.pcid_switches = 0;
    memory_adv_info.invpcid_calls = 0;
    
    for (u8 i = 0; i < 64; i++) {
        memory_adv_info.pcid_table[i].tlb_flushes = 0;
    }
}

u8 memory_adv_find_free_pcid_slot(void) {
    for (u8 i = 0; i < 64; i++) {
        if (!memory_adv_info.pcid_table[i].active) {
            return i;
        }
    }
    return 0xFF;
}

u8 memory_adv_find_free_mtrr_slot(void) {
    for (u8 i = 0; i < 16; i++) {
        if (!memory_adv_info.mtrr_entries[i].enabled) {
            return i;
        }
    }
    return 0xFF;
}

void memory_adv_set_pcid_global(u16 pcid, u8 global) {
    for (u8 i = 0; i < 64; i++) {
        if (memory_adv_info.pcid_table[i].pcid == pcid && memory_adv_info.pcid_table[i].active) {
            memory_adv_info.pcid_table[i].global = global;
            break;
        }
    }
}

u8 memory_adv_is_pcid_global(u16 pcid) {
    for (u8 i = 0; i < 64; i++) {
        if (memory_adv_info.pcid_table[i].pcid == pcid && memory_adv_info.pcid_table[i].active) {
            return memory_adv_info.pcid_table[i].global;
        }
    }
    return 0;
}
