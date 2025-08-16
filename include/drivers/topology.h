/*
 * topology.h – x86 CPU topology detection and NUMA support
 */

#ifndef TOPOLOGY_H
#define TOPOLOGY_H

#include "kernel/types.h"

/* Core functions */
void topology_init(void);

/* Support detection */
u8 topology_is_supported(void);
u8 topology_is_numa_supported(void);

/* CPU topology information */
u8 topology_get_num_packages(void);
u8 topology_get_num_cores_per_package(void);
u8 topology_get_num_threads_per_core(void);
u8 topology_get_num_logical_processors(void);

/* Current CPU information */
u8 topology_get_current_package_id(void);
u8 topology_get_current_core_id(void);
u8 topology_get_current_thread_id(void);
u8 topology_get_current_numa_node(void);
u32 topology_get_current_x2apic_id(void);

/* Cache topology */
u8 topology_get_cache_sharing_level1(void);
u8 topology_get_cache_sharing_level2(void);
u8 topology_get_cache_sharing_level3(void);

/* NUMA information */
u8 topology_get_num_numa_nodes(void);
u64 topology_get_numa_memory_base(u8 node_id);
u64 topology_get_numa_memory_size(u8 node_id);
u8 topology_is_numa_node_enabled(u8 node_id);

#endif
