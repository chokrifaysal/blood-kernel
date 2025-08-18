/*
 * cpu_topology.h – x86 CPU topology detection (cores/threads/packages)
 */

#ifndef CPU_TOPOLOGY_H
#define CPU_TOPOLOGY_H

#include "kernel/types.h"

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

/* Core functions */
void cpu_topology_init(void);

/* Support detection */
u8 cpu_topology_is_supported(void);
u8 cpu_topology_is_x2apic_topology(void);
u8 cpu_topology_is_hybrid_topology(void);

/* Topology information */
u32 cpu_topology_get_num_packages(void);
u32 cpu_topology_get_num_dies(void);
u32 cpu_topology_get_num_cores(void);
u32 cpu_topology_get_num_threads(void);
u32 cpu_topology_get_threads_per_core(void);
u32 cpu_topology_get_cores_per_package(void);

/* CPU information */
const cpu_info_t* cpu_topology_get_cpu_info(u32 cpu_index);
const topology_level_t* cpu_topology_get_level_info(u8 level);
u8 cpu_topology_get_num_levels(void);
u32 cpu_topology_get_bsp_apic_id(void);

/* NUMA and cache information */
u8 cpu_topology_get_numa_nodes(void);
u32 cpu_topology_get_cache_line_size(void);
u32 cpu_topology_get_cache_alignment(void);

/* CPU management */
u8 cpu_topology_add_cpu(u32 apic_id);
u32 cpu_topology_get_max_apic_id(void);
void cpu_topology_update_max_apic_id(u32 apic_id);

/* Topology queries */
u8 cpu_topology_is_same_package(u32 apic_id1, u32 apic_id2);
u8 cpu_topology_is_same_core(u32 apic_id1, u32 apic_id2);
u8 cpu_topology_shares_cache(u32 apic_id1, u32 apic_id2, u8 cache_level);

/* Utilities */
const char* cpu_topology_get_level_name(u8 level_type);
void cpu_topology_print_info(void);

#endif
