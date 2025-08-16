/*
 * topology.c – x86 CPU topology detection and NUMA support
 */

#include "kernel/types.h"

/* CPUID topology leaves */
#define CPUID_TOPOLOGY_LEAF     0x0B
#define CPUID_EXTENDED_TOPOLOGY 0x1F

/* Topology level types */
#define TOPOLOGY_LEVEL_INVALID  0
#define TOPOLOGY_LEVEL_SMT      1  /* Simultaneous Multi-Threading */
#define TOPOLOGY_LEVEL_CORE     2  /* Core */
#define TOPOLOGY_LEVEL_MODULE   3  /* Module */
#define TOPOLOGY_LEVEL_TILE     4  /* Tile */
#define TOPOLOGY_LEVEL_DIE      5  /* Die */
#define TOPOLOGY_LEVEL_DIEGRP   6  /* Die Group */
#define TOPOLOGY_LEVEL_PACKAGE  7  /* Package */

typedef struct {
    u8 level_type;
    u8 level_shift;
    u16 num_logical_processors;
    u32 x2apic_id;
} topology_level_t;

typedef struct {
    u32 apic_id;
    u32 x2apic_id;
    u8 package_id;
    u8 die_id;
    u8 core_id;
    u8 thread_id;
    u8 numa_node;
    u8 cache_level1_shared;
    u8 cache_level2_shared;
    u8 cache_level3_shared;
} cpu_info_t;

typedef struct {
    u8 numa_node_id;
    u32 memory_base_low;
    u32 memory_base_high;
    u32 memory_length_low;
    u32 memory_length_high;
    u8 proximity_domain;
    u8 enabled;
} numa_node_t;

typedef struct {
    u8 topology_supported;
    u8 extended_topology_supported;
    u8 numa_supported;
    u8 num_packages;
    u8 num_dies;
    u8 num_cores_per_package;
    u8 num_threads_per_core;
    u8 num_logical_processors;
    u8 num_numa_nodes;
    topology_level_t levels[8];
    cpu_info_t cpus[256];
    numa_node_t numa_nodes[64];
    u8 current_cpu_id;
} topology_info_t;

static topology_info_t topology_info;

extern u64 msr_read(u32 msr);
extern void msr_write(u32 msr, u64 value);
extern u8 msr_is_supported(void);

static u8 topology_check_support(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check for topology enumeration support */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0));
    
    if (eax >= CPUID_TOPOLOGY_LEAF) {
        __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(CPUID_TOPOLOGY_LEAF), "c"(0));
        topology_info.topology_supported = (ebx != 0);
    }
    
    if (eax >= CPUID_EXTENDED_TOPOLOGY) {
        __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(CPUID_EXTENDED_TOPOLOGY), "c"(0));
        topology_info.extended_topology_supported = (ebx != 0);
    }
    
    return topology_info.topology_supported || topology_info.extended_topology_supported;
}

static void topology_enumerate_levels(void) {
    u32 leaf = topology_info.extended_topology_supported ? CPUID_EXTENDED_TOPOLOGY : CPUID_TOPOLOGY_LEAF;
    u8 level = 0;
    
    while (level < 8) {
        u32 eax, ebx, ecx, edx;
        __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(leaf), "c"(level));
        
        if (ebx == 0) break;
        
        topology_info.levels[level].level_type = (ecx >> 8) & 0xFF;
        topology_info.levels[level].level_shift = eax & 0x1F;
        topology_info.levels[level].num_logical_processors = ebx & 0xFFFF;
        topology_info.levels[level].x2apic_id = edx;
        
        level++;
    }
}

static void topology_detect_cpu_info(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Get initial APIC ID */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    u32 initial_apic_id = (ebx >> 24) & 0xFF;
    
    /* Get x2APIC ID if available */
    u32 x2apic_id = initial_apic_id;
    if (topology_info.topology_supported || topology_info.extended_topology_supported) {
        u32 leaf = topology_info.extended_topology_supported ? CPUID_EXTENDED_TOPOLOGY : CPUID_TOPOLOGY_LEAF;
        __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(leaf), "c"(0));
        x2apic_id = edx;
    }
    
    cpu_info_t* cpu = &topology_info.cpus[topology_info.current_cpu_id];
    cpu->apic_id = initial_apic_id;
    cpu->x2apic_id = x2apic_id;
    
    /* Extract topology information from x2APIC ID */
    u8 smt_shift = 0, core_shift = 0, package_shift = 0;
    
    for (u8 i = 0; i < 8; i++) {
        if (topology_info.levels[i].level_type == TOPOLOGY_LEVEL_SMT) {
            smt_shift = topology_info.levels[i].level_shift;
        } else if (topology_info.levels[i].level_type == TOPOLOGY_LEVEL_CORE) {
            core_shift = topology_info.levels[i].level_shift;
        } else if (topology_info.levels[i].level_type == TOPOLOGY_LEVEL_PACKAGE) {
            package_shift = topology_info.levels[i].level_shift;
        }
    }
    
    if (smt_shift > 0) {
        cpu->thread_id = x2apic_id & ((1 << smt_shift) - 1);
        topology_info.num_threads_per_core = 1 << smt_shift;
    } else {
        cpu->thread_id = 0;
        topology_info.num_threads_per_core = 1;
    }
    
    if (core_shift > smt_shift) {
        cpu->core_id = (x2apic_id >> smt_shift) & ((1 << (core_shift - smt_shift)) - 1);
        topology_info.num_cores_per_package = 1 << (core_shift - smt_shift);
    } else {
        cpu->core_id = 0;
        topology_info.num_cores_per_package = 1;
    }
    
    if (package_shift > core_shift) {
        cpu->package_id = x2apic_id >> core_shift;
    } else {
        cpu->package_id = x2apic_id >> package_shift;
    }
    
    cpu->die_id = 0; /* Simplified - would need more complex detection */
    cpu->numa_node = 0; /* Will be updated by NUMA detection */
}

static void topology_detect_cache_topology(void) {
    cpu_info_t* cpu = &topology_info.cpus[topology_info.current_cpu_id];
    
    /* Detect cache sharing using CPUID leaf 4 */
    for (u32 cache_level = 0; cache_level < 4; cache_level++) {
        u32 eax, ebx, ecx, edx;
        __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(4), "c"(cache_level));
        
        u8 cache_type = eax & 0x1F;
        if (cache_type == 0) break; /* No more cache levels */
        
        u8 cache_level_num = (eax >> 5) & 0x7;
        u32 max_cores_sharing = ((eax >> 14) & 0xFFF) + 1;
        
        switch (cache_level_num) {
            case 1:
                cpu->cache_level1_shared = max_cores_sharing;
                break;
            case 2:
                cpu->cache_level2_shared = max_cores_sharing;
                break;
            case 3:
                cpu->cache_level3_shared = max_cores_sharing;
                break;
        }
    }
}

static void topology_parse_srat_table(void) {
    /* Simplified SRAT parsing - would need ACPI integration */
    extern void* acpi_find_table(const char* signature);
    void* srat_table = acpi_find_table("SRAT");
    
    if (!srat_table) {
        topology_info.numa_supported = 0;
        topology_info.num_numa_nodes = 1;
        
        /* Single NUMA node covering all memory */
        topology_info.numa_nodes[0].numa_node_id = 0;
        topology_info.numa_nodes[0].memory_base_low = 0;
        topology_info.numa_nodes[0].memory_base_high = 0;
        topology_info.numa_nodes[0].memory_length_low = 0x100000000ULL; /* 4GB */
        topology_info.numa_nodes[0].memory_length_high = 0;
        topology_info.numa_nodes[0].proximity_domain = 0;
        topology_info.numa_nodes[0].enabled = 1;
        return;
    }
    
    topology_info.numa_supported = 1;
    topology_info.num_numa_nodes = 0;
    
    /* Parse SRAT entries */
    u8* srat_data = (u8*)srat_table;
    u32 srat_length = *(u32*)(srat_data + 4);
    u32 offset = 48; /* Skip SRAT header */
    
    while (offset < srat_length && topology_info.num_numa_nodes < 64) {
        u8 entry_type = srat_data[offset];
        u8 entry_length = srat_data[offset + 1];
        
        if (entry_type == 1) { /* Memory Affinity Structure */
            numa_node_t* node = &topology_info.numa_nodes[topology_info.num_numa_nodes];
            
            node->proximity_domain = *(u32*)(srat_data + offset + 2);
            node->memory_base_low = *(u32*)(srat_data + offset + 8);
            node->memory_base_high = *(u32*)(srat_data + offset + 12);
            node->memory_length_low = *(u32*)(srat_data + offset + 16);
            node->memory_length_high = *(u32*)(srat_data + offset + 20);
            node->enabled = (*(u32*)(srat_data + offset + 28) & 1) != 0;
            node->numa_node_id = topology_info.num_numa_nodes;
            
            if (node->enabled) {
                topology_info.num_numa_nodes++;
            }
        }
        
        offset += entry_length;
    }
    
    if (topology_info.num_numa_nodes == 0) {
        topology_info.numa_supported = 0;
        topology_info.num_numa_nodes = 1;
    }
}

void topology_init(void) {
    topology_info.topology_supported = 0;
    topology_info.extended_topology_supported = 0;
    topology_info.numa_supported = 0;
    topology_info.num_packages = 1;
    topology_info.num_dies = 1;
    topology_info.num_cores_per_package = 1;
    topology_info.num_threads_per_core = 1;
    topology_info.num_logical_processors = 1;
    topology_info.current_cpu_id = 0;
    
    if (!topology_check_support()) {
        return;
    }
    
    topology_enumerate_levels();
    topology_detect_cpu_info();
    topology_detect_cache_topology();
    topology_parse_srat_table();
    
    /* Update global counters */
    topology_info.num_logical_processors = topology_info.num_packages * 
                                          topology_info.num_cores_per_package * 
                                          topology_info.num_threads_per_core;
}

u8 topology_is_supported(void) {
    return topology_info.topology_supported || topology_info.extended_topology_supported;
}

u8 topology_is_numa_supported(void) {
    return topology_info.numa_supported;
}

u8 topology_get_num_packages(void) {
    return topology_info.num_packages;
}

u8 topology_get_num_cores_per_package(void) {
    return topology_info.num_cores_per_package;
}

u8 topology_get_num_threads_per_core(void) {
    return topology_info.num_threads_per_core;
}

u8 topology_get_num_logical_processors(void) {
    return topology_info.num_logical_processors;
}

u8 topology_get_num_numa_nodes(void) {
    return topology_info.num_numa_nodes;
}

u8 topology_get_current_package_id(void) {
    return topology_info.cpus[topology_info.current_cpu_id].package_id;
}

u8 topology_get_current_core_id(void) {
    return topology_info.cpus[topology_info.current_cpu_id].core_id;
}

u8 topology_get_current_thread_id(void) {
    return topology_info.cpus[topology_info.current_cpu_id].thread_id;
}

u8 topology_get_current_numa_node(void) {
    return topology_info.cpus[topology_info.current_cpu_id].numa_node;
}

u32 topology_get_current_x2apic_id(void) {
    return topology_info.cpus[topology_info.current_cpu_id].x2apic_id;
}

u8 topology_get_cache_sharing_level1(void) {
    return topology_info.cpus[topology_info.current_cpu_id].cache_level1_shared;
}

u8 topology_get_cache_sharing_level2(void) {
    return topology_info.cpus[topology_info.current_cpu_id].cache_level2_shared;
}

u8 topology_get_cache_sharing_level3(void) {
    return topology_info.cpus[topology_info.current_cpu_id].cache_level3_shared;
}

u64 topology_get_numa_memory_base(u8 node_id) {
    if (node_id >= topology_info.num_numa_nodes) return 0;
    
    numa_node_t* node = &topology_info.numa_nodes[node_id];
    return ((u64)node->memory_base_high << 32) | node->memory_base_low;
}

u64 topology_get_numa_memory_size(u8 node_id) {
    if (node_id >= topology_info.num_numa_nodes) return 0;
    
    numa_node_t* node = &topology_info.numa_nodes[node_id];
    return ((u64)node->memory_length_high << 32) | node->memory_length_low;
}

u8 topology_is_numa_node_enabled(u8 node_id) {
    if (node_id >= topology_info.num_numa_nodes) return 0;
    
    return topology_info.numa_nodes[node_id].enabled;
}
