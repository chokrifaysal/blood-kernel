/*
 * numa.h – x86 NUMA memory management and affinity
 */

#ifndef NUMA_H
#define NUMA_H

#include "kernel/types.h"

/* NUMA allocation policies */
#define NUMA_POLICY_DEFAULT     0
#define NUMA_POLICY_BIND        1
#define NUMA_POLICY_INTERLEAVE  2
#define NUMA_POLICY_PREFERRED   3

/* Core functions */
void numa_init(void);

/* Status functions */
u8 numa_is_enabled(void);
u8 numa_get_num_nodes(void);
u8 numa_get_current_node(void);
void numa_set_current_node(u8 node_id);

/* Node information */
u8 numa_get_distance(u8 from_node, u8 to_node);
u64 numa_get_node_memory_size(u8 node_id);
u64 numa_get_node_free_memory(u8 node_id);
u32 numa_get_node_processor_count(u8 node_id);

/* Node selection */
u8 numa_find_closest_node(u8 from_node);
u8 numa_find_node_with_most_memory(void);

/* Allocation policy */
void numa_set_allocation_policy(u32 policy);
u32 numa_get_allocation_policy(void);
void numa_set_interleave_mask(u8 mask);
u8 numa_get_interleave_mask(void);

/* Memory allocation */
void* numa_alloc(u32 size);
void* numa_alloc_on_node(u32 size, u8 node_id);
void* numa_alloc_preferred(u32 size, u8 preferred_node);
void* numa_alloc_interleaved(u32 size);
void numa_free(void* ptr, u32 size);

/* Page management */
u8 numa_get_page_node(void* page_addr);
void numa_migrate_pages(void* start_addr, u32 page_count, u8 target_node);

#endif
