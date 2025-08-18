/*
 * cpu_topology.c – x86 CPU topology detection (cores/threads/packages)
 */

#include "kernel/types.h"

/* APIC ID extraction masks */
#define APIC_ID_MASK                    0xFF000000
#define APIC_ID_SHIFT                   24

/* Topology level types */
#define TOPOLOGY_LEVEL_INVALID          0
#define TOPOLOGY_LEVEL_SMT              1  /* Simultaneous Multi-Threading */
#define TOPOLOGY_LEVEL_CORE             2  /* Physical Core */
#define TOPOLOGY_LEVEL_MODULE           3  /* Module/Tile */
#define TOPOLOGY_LEVEL_TILE             4  /* Tile */
#define TOPOLOGY_LEVEL_DIE              5  /* Die */
#define TOPOLOGY_LEVEL_PACKAGE          6  /* Package/Socket */

/* Cache sharing levels */
#define CACHE_SHARING_L1                1
#define CACHE_SHARING_L2                2
#define CACHE_SHARING_L3                3
#define CACHE_SHARING_LLC               4  /* Last Level Cache */

typedef struct {
    u32 apic_id;
    u32 package_id;
    u32 die_id;
    u32 tile_id;
    u32 module_id;
    u32 core_id;
    u32 thread_id;
    u8 is_bsp;  /* Bootstrap Processor */
    u8 enabled;
    u32 cache_sharing_mask[4];  /* L1, L2, L3, LLC */
} cpu_info_t;

typedef struct {
    u8 level_type;
    u8 level_shift;
    u32 num_units;
    u32 next_level_apic_id;
} topology_level_t;

typedef struct {
    u8 cpu_topology_supported;
    u8 x2apic_topology;
    u8 hybrid_topology;  /* P-cores + E-cores */
    u32 num_packages;
    u32 num_dies;
    u32 num_tiles;
    u32 num_modules;
    u32 num_cores;
    u32 num_threads;
    u32 threads_per_core;
    u32 cores_per_package;
    u32 max_apic_id;
    cpu_info_t cpus[256];
    topology_level_t levels[8];
    u8 num_levels;
    u32 bsp_apic_id;
    u8 numa_nodes;
    u32 cache_line_size;
    u32 cache_alignment;
} cpu_topology_info_t;

static cpu_topology_info_t cpu_topology_info;

extern u32 apic_get_id(void);
extern u8 apic_is_bsp(void);

static void cpu_topology_detect_levels(void) {
    u32 eax, ebx, ecx, edx;
    u8 level = 0;
    
    /* Use CPUID leaf 0xB (Extended Topology Enumeration) */
    for (u8 i = 0; i < 8; i++) {
        __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0xB), "c"(i));
        
        u8 level_type = (ecx >> 8) & 0xFF;
        if (level_type == TOPOLOGY_LEVEL_INVALID) break;
        
        cpu_topology_info.levels[level].level_type = level_type;
        cpu_topology_info.levels[level].level_shift = eax & 0x1F;
        cpu_topology_info.levels[level].num_units = ebx & 0xFFFF;
        cpu_topology_info.levels[level].next_level_apic_id = edx;
        
        level++;
    }
    
    cpu_topology_info.num_levels = level;
    
    /* Fallback to legacy CPUID if extended topology not available */
    if (level == 0) {
        __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
        
        u32 logical_processors = (ebx >> 16) & 0xFF;
        u32 initial_apic_id = (ebx >> 24) & 0xFF;
        
        /* Check for Hyper-Threading */
        if (edx & (1 << 28)) {
            cpu_topology_info.threads_per_core = logical_processors;
        } else {
            cpu_topology_info.threads_per_core = 1;
        }
        
        /* Simple topology for legacy systems */
        cpu_topology_info.levels[0].level_type = TOPOLOGY_LEVEL_SMT;
        cpu_topology_info.levels[0].level_shift = 1;
        cpu_topology_info.levels[0].num_units = cpu_topology_info.threads_per_core;
        
        cpu_topology_info.levels[1].level_type = TOPOLOGY_LEVEL_PACKAGE;
        cpu_topology_info.levels[1].level_shift = 8;
        cpu_topology_info.levels[1].num_units = 1;
        
        cpu_topology_info.num_levels = 2;
    }
}

static void cpu_topology_parse_apic_id(u32 apic_id, cpu_info_t* cpu) {
    cpu->apic_id = apic_id;
    
    /* Extract topology information from APIC ID */
    u32 remaining_id = apic_id;
    
    for (u8 i = 0; i < cpu_topology_info.num_levels; i++) {
        u32 mask = (1 << cpu_topology_info.levels[i].level_shift) - 1;
        u32 id_at_level = remaining_id & mask;
        
        switch (cpu_topology_info.levels[i].level_type) {
            case TOPOLOGY_LEVEL_SMT:
                cpu->thread_id = id_at_level;
                break;
            case TOPOLOGY_LEVEL_CORE:
                cpu->core_id = id_at_level;
                break;
            case TOPOLOGY_LEVEL_MODULE:
                cpu->module_id = id_at_level;
                break;
            case TOPOLOGY_LEVEL_TILE:
                cpu->tile_id = id_at_level;
                break;
            case TOPOLOGY_LEVEL_DIE:
                cpu->die_id = id_at_level;
                break;
            case TOPOLOGY_LEVEL_PACKAGE:
                cpu->package_id = id_at_level;
                break;
        }
        
        remaining_id >>= cpu_topology_info.levels[i].level_shift;
    }
}

static void cpu_topology_detect_cache_sharing(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Enumerate cache levels and sharing */
    for (u8 i = 0; i < 4; i++) {
        __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(4), "c"(i));
        
        u8 cache_type = eax & 0x1F;
        if (cache_type == 0) break;  /* No more cache levels */
        
        u32 max_cores_sharing = ((eax >> 14) & 0xFFF) + 1;
        u32 max_threads_sharing = ((eax >> 26) & 0x3F) + 1;
        
        /* Store cache sharing information */
        for (u32 j = 0; j < cpu_topology_info.num_threads; j++) {
            if (cpu_topology_info.cpus[j].enabled) {
                cpu_topology_info.cpus[j].cache_sharing_mask[i] = max_cores_sharing;
            }
        }
    }
}

void cpu_topology_init(void) {
    cpu_topology_info.cpu_topology_supported = 0;
    cpu_topology_info.x2apic_topology = 0;
    cpu_topology_info.hybrid_topology = 0;
    cpu_topology_info.num_packages = 0;
    cpu_topology_info.num_dies = 0;
    cpu_topology_info.num_tiles = 0;
    cpu_topology_info.num_modules = 0;
    cpu_topology_info.num_cores = 0;
    cpu_topology_info.num_threads = 0;
    cpu_topology_info.threads_per_core = 1;
    cpu_topology_info.cores_per_package = 1;
    cpu_topology_info.max_apic_id = 0;
    cpu_topology_info.num_levels = 0;
    cpu_topology_info.bsp_apic_id = 0;
    cpu_topology_info.numa_nodes = 1;
    cpu_topology_info.cache_line_size = 64;
    cpu_topology_info.cache_alignment = 64;
    
    for (u32 i = 0; i < 256; i++) {
        cpu_topology_info.cpus[i].apic_id = 0;
        cpu_topology_info.cpus[i].package_id = 0;
        cpu_topology_info.cpus[i].die_id = 0;
        cpu_topology_info.cpus[i].tile_id = 0;
        cpu_topology_info.cpus[i].module_id = 0;
        cpu_topology_info.cpus[i].core_id = 0;
        cpu_topology_info.cpus[i].thread_id = 0;
        cpu_topology_info.cpus[i].is_bsp = 0;
        cpu_topology_info.cpus[i].enabled = 0;
        
        for (u8 j = 0; j < 4; j++) {
            cpu_topology_info.cpus[i].cache_sharing_mask[j] = 0;
        }
    }
    
    for (u8 i = 0; i < 8; i++) {
        cpu_topology_info.levels[i].level_type = TOPOLOGY_LEVEL_INVALID;
        cpu_topology_info.levels[i].level_shift = 0;
        cpu_topology_info.levels[i].num_units = 0;
        cpu_topology_info.levels[i].next_level_apic_id = 0;
    }
    
    /* Check for topology enumeration support */
    u32 eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0));
    
    if (eax >= 0xB) {
        cpu_topology_info.cpu_topology_supported = 1;
        cpu_topology_detect_levels();
    }
    
    /* Check for x2APIC support */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    cpu_topology_info.x2apic_topology = (ecx & (1 << 21)) != 0;
    
    /* Check for hybrid topology (Alder Lake+) */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    cpu_topology_info.hybrid_topology = (edx & (1 << 15)) != 0;
    
    /* Get current CPU information */
    u32 current_apic_id = apic_get_id();
    cpu_topology_info.bsp_apic_id = current_apic_id;
    
    /* Initialize current CPU */
    cpu_info_t* current_cpu = &cpu_topology_info.cpus[0];
    current_cpu->enabled = 1;
    current_cpu->is_bsp = apic_is_bsp();
    cpu_topology_parse_apic_id(current_apic_id, current_cpu);
    
    cpu_topology_info.num_threads = 1;
    cpu_topology_info.num_cores = 1;
    cpu_topology_info.num_packages = 1;
    
    /* Detect cache sharing */
    cpu_topology_detect_cache_sharing();
    
    /* Calculate derived values */
    if (cpu_topology_info.num_levels > 0) {
        for (u8 i = 0; i < cpu_topology_info.num_levels; i++) {
            if (cpu_topology_info.levels[i].level_type == TOPOLOGY_LEVEL_SMT) {
                cpu_topology_info.threads_per_core = cpu_topology_info.levels[i].num_units;
            }
        }
    }
    
    cpu_topology_info.cores_per_package = cpu_topology_info.num_cores / cpu_topology_info.num_packages;
}

u8 cpu_topology_is_supported(void) {
    return cpu_topology_info.cpu_topology_supported;
}

u8 cpu_topology_is_x2apic_topology(void) {
    return cpu_topology_info.x2apic_topology;
}

u8 cpu_topology_is_hybrid_topology(void) {
    return cpu_topology_info.hybrid_topology;
}

u32 cpu_topology_get_num_packages(void) {
    return cpu_topology_info.num_packages;
}

u32 cpu_topology_get_num_dies(void) {
    return cpu_topology_info.num_dies;
}

u32 cpu_topology_get_num_cores(void) {
    return cpu_topology_info.num_cores;
}

u32 cpu_topology_get_num_threads(void) {
    return cpu_topology_info.num_threads;
}

u32 cpu_topology_get_threads_per_core(void) {
    return cpu_topology_info.threads_per_core;
}

u32 cpu_topology_get_cores_per_package(void) {
    return cpu_topology_info.cores_per_package;
}

const cpu_info_t* cpu_topology_get_cpu_info(u32 cpu_index) {
    if (cpu_index >= 256 || !cpu_topology_info.cpus[cpu_index].enabled) return 0;
    return &cpu_topology_info.cpus[cpu_index];
}

const topology_level_t* cpu_topology_get_level_info(u8 level) {
    if (level >= cpu_topology_info.num_levels) return 0;
    return &cpu_topology_info.levels[level];
}

u8 cpu_topology_get_num_levels(void) {
    return cpu_topology_info.num_levels;
}

u32 cpu_topology_get_bsp_apic_id(void) {
    return cpu_topology_info.bsp_apic_id;
}

u8 cpu_topology_get_numa_nodes(void) {
    return cpu_topology_info.numa_nodes;
}

u32 cpu_topology_get_cache_line_size(void) {
    return cpu_topology_info.cache_line_size;
}

u32 cpu_topology_get_cache_alignment(void) {
    return cpu_topology_info.cache_alignment;
}

u8 cpu_topology_add_cpu(u32 apic_id) {
    if (cpu_topology_info.num_threads >= 256) return 0;
    
    cpu_info_t* cpu = &cpu_topology_info.cpus[cpu_topology_info.num_threads];
    cpu->enabled = 1;
    cpu->is_bsp = (apic_id == cpu_topology_info.bsp_apic_id);
    cpu_topology_parse_apic_id(apic_id, cpu);
    
    cpu_topology_info.num_threads++;
    
    /* Update package/core counts */
    u32 max_package = 0, max_core = 0;
    for (u32 i = 0; i < cpu_topology_info.num_threads; i++) {
        if (cpu_topology_info.cpus[i].enabled) {
            if (cpu_topology_info.cpus[i].package_id > max_package) {
                max_package = cpu_topology_info.cpus[i].package_id;
            }
            if (cpu_topology_info.cpus[i].core_id > max_core) {
                max_core = cpu_topology_info.cpus[i].core_id;
            }
        }
    }
    
    cpu_topology_info.num_packages = max_package + 1;
    cpu_topology_info.num_cores = max_core + 1;
    cpu_topology_info.cores_per_package = cpu_topology_info.num_cores / cpu_topology_info.num_packages;
    
    return 1;
}

u8 cpu_topology_is_same_package(u32 apic_id1, u32 apic_id2) {
    cpu_info_t cpu1, cpu2;
    cpu_topology_parse_apic_id(apic_id1, &cpu1);
    cpu_topology_parse_apic_id(apic_id2, &cpu2);
    
    return cpu1.package_id == cpu2.package_id;
}

u8 cpu_topology_is_same_core(u32 apic_id1, u32 apic_id2) {
    cpu_info_t cpu1, cpu2;
    cpu_topology_parse_apic_id(apic_id1, &cpu1);
    cpu_topology_parse_apic_id(apic_id2, &cpu2);
    
    return (cpu1.package_id == cpu2.package_id) && (cpu1.core_id == cpu2.core_id);
}

u8 cpu_topology_shares_cache(u32 apic_id1, u32 apic_id2, u8 cache_level) {
    if (cache_level >= 4) return 0;
    
    const cpu_info_t* cpu1 = 0;
    const cpu_info_t* cpu2 = 0;
    
    /* Find CPUs by APIC ID */
    for (u32 i = 0; i < cpu_topology_info.num_threads; i++) {
        if (cpu_topology_info.cpus[i].enabled) {
            if (cpu_topology_info.cpus[i].apic_id == apic_id1) cpu1 = &cpu_topology_info.cpus[i];
            if (cpu_topology_info.cpus[i].apic_id == apic_id2) cpu2 = &cpu_topology_info.cpus[i];
        }
    }
    
    if (!cpu1 || !cpu2) return 0;
    
    return cpu1->cache_sharing_mask[cache_level] == cpu2->cache_sharing_mask[cache_level];
}

const char* cpu_topology_get_level_name(u8 level_type) {
    switch (level_type) {
        case TOPOLOGY_LEVEL_SMT: return "SMT Thread";
        case TOPOLOGY_LEVEL_CORE: return "Physical Core";
        case TOPOLOGY_LEVEL_MODULE: return "Module";
        case TOPOLOGY_LEVEL_TILE: return "Tile";
        case TOPOLOGY_LEVEL_DIE: return "Die";
        case TOPOLOGY_LEVEL_PACKAGE: return "Package";
        default: return "Unknown";
    }
}

void cpu_topology_print_info(void) {
    /* This would print topology information - placeholder for demo */
}

u32 cpu_topology_get_max_apic_id(void) {
    return cpu_topology_info.max_apic_id;
}

void cpu_topology_update_max_apic_id(u32 apic_id) {
    if (apic_id > cpu_topology_info.max_apic_id) {
        cpu_topology_info.max_apic_id = apic_id;
    }
}
