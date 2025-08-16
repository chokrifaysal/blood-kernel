/*
 * numa.c – x86 NUMA memory management and affinity
 */

#include "kernel/types.h"

/* NUMA distance matrix values */
#define NUMA_LOCAL_DISTANCE     10
#define NUMA_REMOTE_DISTANCE    20
#define NUMA_UNREACHABLE        255

typedef struct {
    u64 base_address;
    u64 size;
    u8 node_id;
    u8 enabled;
    u8 hot_pluggable;
    u8 non_volatile;
} numa_memory_range_t;

typedef struct {
    u8 node_id;
    u32 processor_count;
    u32 processor_list[64];
    u64 total_memory;
    u64 free_memory;
    u32 memory_range_count;
    numa_memory_range_t memory_ranges[16];
    u8 distance_table[64];
} numa_node_info_t;

typedef struct {
    u8 numa_enabled;
    u8 num_nodes;
    u8 current_node;
    numa_node_info_t nodes[64];
    u8 distance_matrix[64][64];
    u32 allocation_policy;
    u8 interleave_mask;
} numa_info_t;

static numa_info_t numa_info;

/* NUMA allocation policies */
#define NUMA_POLICY_DEFAULT     0
#define NUMA_POLICY_BIND        1
#define NUMA_POLICY_INTERLEAVE  2
#define NUMA_POLICY_PREFERRED   3

extern void* topology_get_numa_memory_base(u8 node_id);
extern u64 topology_get_numa_memory_size(u8 node_id);
extern u8 topology_is_numa_node_enabled(u8 node_id);
extern u8 topology_get_num_numa_nodes(void);
extern u8 topology_is_numa_supported(void);

static void numa_parse_slit_table(void) {
    extern void* acpi_find_table(const char* signature);
    void* slit_table = acpi_find_table("SLIT");
    
    if (!slit_table) {
        /* Create default distance matrix */
        for (u8 i = 0; i < numa_info.num_nodes; i++) {
            for (u8 j = 0; j < numa_info.num_nodes; j++) {
                if (i == j) {
                    numa_info.distance_matrix[i][j] = NUMA_LOCAL_DISTANCE;
                } else {
                    numa_info.distance_matrix[i][j] = NUMA_REMOTE_DISTANCE;
                }
            }
        }
        return;
    }
    
    u8* slit_data = (u8*)slit_table;
    u64 num_localities = *(u64*)(slit_data + 36);
    u8* distances = slit_data + 44;
    
    for (u8 i = 0; i < numa_info.num_nodes && i < num_localities; i++) {
        for (u8 j = 0; j < numa_info.num_nodes && j < num_localities; j++) {
            numa_info.distance_matrix[i][j] = distances[i * num_localities + j];
        }
    }
}

static void numa_detect_memory_ranges(void) {
    for (u8 node = 0; node < numa_info.num_nodes; node++) {
        numa_node_info_t* node_info = &numa_info.nodes[node];
        
        u64 base = (u64)topology_get_numa_memory_base(node);
        u64 size = topology_get_numa_memory_size(node);
        
        if (size > 0) {
            numa_memory_range_t* range = &node_info->memory_ranges[0];
            range->base_address = base;
            range->size = size;
            range->node_id = node;
            range->enabled = topology_is_numa_node_enabled(node);
            range->hot_pluggable = 0;
            range->non_volatile = 0;
            
            node_info->memory_range_count = 1;
            node_info->total_memory = size;
            node_info->free_memory = size;
        }
    }
}

static void numa_detect_processor_affinity(void) {
    /* Simplified processor affinity detection */
    extern u8 topology_get_current_numa_node(void);
    
    u8 current_node = topology_get_current_numa_node();
    if (current_node < numa_info.num_nodes) {
        numa_node_info_t* node_info = &numa_info.nodes[current_node];
        
        if (node_info->processor_count < 64) {
            extern u32 topology_get_current_x2apic_id(void);
            u32 apic_id = topology_get_current_x2apic_id();
            
            node_info->processor_list[node_info->processor_count] = apic_id;
            node_info->processor_count++;
        }
    }
}

void numa_init(void) {
    numa_info.numa_enabled = 0;
    numa_info.num_nodes = 1;
    numa_info.current_node = 0;
    numa_info.allocation_policy = NUMA_POLICY_DEFAULT;
    numa_info.interleave_mask = 0x1;
    
    if (!topology_is_numa_supported()) {
        /* Single node system */
        numa_node_info_t* node = &numa_info.nodes[0];
        node->node_id = 0;
        node->processor_count = 0;
        node->total_memory = 0x100000000ULL; /* 4GB default */
        node->free_memory = node->total_memory;
        node->memory_range_count = 0;
        
        numa_info.distance_matrix[0][0] = NUMA_LOCAL_DISTANCE;
        return;
    }
    
    numa_info.numa_enabled = 1;
    numa_info.num_nodes = topology_get_num_numa_nodes();
    
    /* Initialize node structures */
    for (u8 i = 0; i < numa_info.num_nodes; i++) {
        numa_node_info_t* node = &numa_info.nodes[i];
        node->node_id = i;
        node->processor_count = 0;
        node->total_memory = 0;
        node->free_memory = 0;
        node->memory_range_count = 0;
    }
    
    numa_parse_slit_table();
    numa_detect_memory_ranges();
    numa_detect_processor_affinity();
}

u8 numa_is_enabled(void) {
    return numa_info.numa_enabled;
}

u8 numa_get_num_nodes(void) {
    return numa_info.num_nodes;
}

u8 numa_get_current_node(void) {
    return numa_info.current_node;
}

u8 numa_get_distance(u8 from_node, u8 to_node) {
    if (from_node >= numa_info.num_nodes || to_node >= numa_info.num_nodes) {
        return NUMA_UNREACHABLE;
    }
    
    return numa_info.distance_matrix[from_node][to_node];
}

u64 numa_get_node_memory_size(u8 node_id) {
    if (node_id >= numa_info.num_nodes) return 0;
    
    return numa_info.nodes[node_id].total_memory;
}

u64 numa_get_node_free_memory(u8 node_id) {
    if (node_id >= numa_info.num_nodes) return 0;
    
    return numa_info.nodes[node_id].free_memory;
}

u32 numa_get_node_processor_count(u8 node_id) {
    if (node_id >= numa_info.num_nodes) return 0;
    
    return numa_info.nodes[node_id].processor_count;
}

u8 numa_find_closest_node(u8 from_node) {
    if (from_node >= numa_info.num_nodes) return 0;
    
    u8 closest_node = from_node;
    u8 min_distance = NUMA_UNREACHABLE;
    
    for (u8 i = 0; i < numa_info.num_nodes; i++) {
        if (i == from_node) continue;
        
        u8 distance = numa_info.distance_matrix[from_node][i];
        if (distance < min_distance && numa_info.nodes[i].free_memory > 0) {
            min_distance = distance;
            closest_node = i;
        }
    }
    
    return closest_node;
}

u8 numa_find_node_with_most_memory(void) {
    u8 best_node = 0;
    u64 max_memory = 0;
    
    for (u8 i = 0; i < numa_info.num_nodes; i++) {
        if (numa_info.nodes[i].free_memory > max_memory) {
            max_memory = numa_info.nodes[i].free_memory;
            best_node = i;
        }
    }
    
    return best_node;
}

void numa_set_allocation_policy(u32 policy) {
    numa_info.allocation_policy = policy;
}

u32 numa_get_allocation_policy(void) {
    return numa_info.allocation_policy;
}

void numa_set_interleave_mask(u8 mask) {
    numa_info.interleave_mask = mask;
}

u8 numa_get_interleave_mask(void) {
    return numa_info.interleave_mask;
}

void* numa_alloc_on_node(u32 size, u8 node_id) {
    if (node_id >= numa_info.num_nodes) return 0;
    if (numa_info.nodes[node_id].free_memory < size) return 0;
    
    extern void* paging_alloc_pages(u32 count);
    u32 pages = (size + 4095) / 4096;
    void* ptr = paging_alloc_pages(pages);
    
    if (ptr) {
        numa_info.nodes[node_id].free_memory -= pages * 4096;
    }
    
    return ptr;
}

void* numa_alloc_preferred(u32 size, u8 preferred_node) {
    /* Try preferred node first */
    void* ptr = numa_alloc_on_node(size, preferred_node);
    if (ptr) return ptr;
    
    /* Try closest nodes */
    for (u8 distance = NUMA_LOCAL_DISTANCE + 1; distance < NUMA_UNREACHABLE; distance++) {
        for (u8 i = 0; i < numa_info.num_nodes; i++) {
            if (numa_info.distance_matrix[preferred_node][i] == distance) {
                ptr = numa_alloc_on_node(size, i);
                if (ptr) return ptr;
            }
        }
    }
    
    return 0;
}

void* numa_alloc_interleaved(u32 size) {
    static u8 next_node = 0;
    
    /* Find next node in interleave mask */
    for (u8 i = 0; i < numa_info.num_nodes; i++) {
        u8 node = (next_node + i) % numa_info.num_nodes;
        
        if (!(numa_info.interleave_mask & (1 << node))) continue;
        
        void* ptr = numa_alloc_on_node(size, node);
        if (ptr) {
            next_node = (node + 1) % numa_info.num_nodes;
            return ptr;
        }
    }
    
    return 0;
}

void* numa_alloc(u32 size) {
    switch (numa_info.allocation_policy) {
        case NUMA_POLICY_BIND:
            return numa_alloc_on_node(size, numa_info.current_node);
            
        case NUMA_POLICY_INTERLEAVE:
            return numa_alloc_interleaved(size);
            
        case NUMA_POLICY_PREFERRED:
            return numa_alloc_preferred(size, numa_info.current_node);
            
        case NUMA_POLICY_DEFAULT:
        default:
            return numa_alloc_preferred(size, numa_info.current_node);
    }
}

void numa_free(void* ptr, u32 size) {
    if (!ptr) return;
    
    extern void paging_free_pages(void* ptr, u32 count);
    u32 pages = (size + 4095) / 4096;
    paging_free_pages(ptr, pages);
    
    /* Update free memory counters */
    for (u8 i = 0; i < numa_info.num_nodes; i++) {
        numa_info.nodes[i].free_memory += pages * 4096;
    }
}

u8 numa_get_page_node(void* page_addr) {
    u64 addr = (u64)page_addr;
    
    for (u8 node = 0; node < numa_info.num_nodes; node++) {
        numa_node_info_t* node_info = &numa_info.nodes[node];
        
        for (u32 range = 0; range < node_info->memory_range_count; range++) {
            numa_memory_range_t* mem_range = &node_info->memory_ranges[range];
            
            if (addr >= mem_range->base_address && 
                addr < mem_range->base_address + mem_range->size) {
                return node;
            }
        }
    }
    
    return 0; /* Default to node 0 */
}

void numa_migrate_pages(void* start_addr, u32 page_count, u8 target_node) {
    /* Simplified page migration */
    if (target_node >= numa_info.num_nodes) return;
    
    /* In a real implementation, this would involve:
     * 1. Allocating pages on target node
     * 2. Copying page contents
     * 3. Updating page tables
     * 4. Freeing old pages
     */
}

void numa_set_current_node(u8 node_id) {
    if (node_id < numa_info.num_nodes) {
        numa_info.current_node = node_id;
    }
}
