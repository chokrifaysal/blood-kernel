/*
 * memory_ordering.c – x86 advanced memory ordering (memory barriers/fencing)
 */

#include "kernel/types.h"

/* Memory ordering types */
#define MEMORY_ORDER_RELAXED            0
#define MEMORY_ORDER_ACQUIRE            1
#define MEMORY_ORDER_RELEASE            2
#define MEMORY_ORDER_ACQ_REL            3
#define MEMORY_ORDER_SEQ_CST            4

/* Fence types */
#define FENCE_TYPE_LOAD                 0
#define FENCE_TYPE_STORE                1
#define FENCE_TYPE_FULL                 2
#define FENCE_TYPE_SERIALIZE            3

/* Memory model features */
#define MEMORY_MODEL_TSO                0  /* Total Store Order (x86 default) */
#define MEMORY_MODEL_PSO                1  /* Partial Store Order */
#define MEMORY_MODEL_RMO                2  /* Relaxed Memory Order */

typedef struct {
    u32 fence_type;
    u64 timestamp;
    u64 instruction_pointer;
    u32 cpu_id;
} fence_record_t;

typedef struct {
    u64 address;
    u64 value;
    u8 ordering;
    u64 timestamp;
    u8 cpu_id;
    u8 operation;  /* 0=load, 1=store */
} memory_access_t;

typedef struct {
    u8 memory_ordering_supported;
    u8 strong_ordering;  /* x86 has strong memory ordering */
    u8 memory_model;
    u32 total_fences;
    u32 load_fences;
    u32 store_fences;
    u32 full_fences;
    u32 serialize_fences;
    fence_record_t fence_history[256];
    u32 fence_history_index;
    memory_access_t access_history[128];
    u32 access_history_index;
    u32 ordering_violations;
    u64 last_fence_time;
    u64 max_fence_latency;
    u64 min_fence_latency;
} memory_ordering_info_t;

static memory_ordering_info_t memory_ordering_info;

extern u64 timer_get_ticks(void);
extern u32 cpu_get_current_id(void);

static void memory_ordering_record_fence(u32 fence_type) {
    fence_record_t* record = &memory_ordering_info.fence_history[memory_ordering_info.fence_history_index];
    
    record->fence_type = fence_type;
    record->timestamp = timer_get_ticks();
    record->cpu_id = cpu_get_current_id();
    
    /* Get instruction pointer */
    __asm__ volatile("lea (%%rip), %0" : "=r"(record->instruction_pointer));
    
    memory_ordering_info.fence_history_index = (memory_ordering_info.fence_history_index + 1) % 256;
    memory_ordering_info.total_fences++;
    memory_ordering_info.last_fence_time = record->timestamp;
}

void memory_ordering_init(void) {
    memory_ordering_info.memory_ordering_supported = 1;  /* x86 always supports memory ordering */
    memory_ordering_info.strong_ordering = 1;  /* x86 has strong memory ordering by default */
    memory_ordering_info.memory_model = MEMORY_MODEL_TSO;  /* x86 uses TSO */
    memory_ordering_info.total_fences = 0;
    memory_ordering_info.load_fences = 0;
    memory_ordering_info.store_fences = 0;
    memory_ordering_info.full_fences = 0;
    memory_ordering_info.serialize_fences = 0;
    memory_ordering_info.fence_history_index = 0;
    memory_ordering_info.access_history_index = 0;
    memory_ordering_info.ordering_violations = 0;
    memory_ordering_info.last_fence_time = 0;
    memory_ordering_info.max_fence_latency = 0;
    memory_ordering_info.min_fence_latency = 0xFFFFFFFFFFFFFFFF;
    
    for (u32 i = 0; i < 256; i++) {
        memory_ordering_info.fence_history[i].fence_type = 0;
        memory_ordering_info.fence_history[i].timestamp = 0;
        memory_ordering_info.fence_history[i].instruction_pointer = 0;
        memory_ordering_info.fence_history[i].cpu_id = 0;
    }
    
    for (u32 i = 0; i < 128; i++) {
        memory_ordering_info.access_history[i].address = 0;
        memory_ordering_info.access_history[i].value = 0;
        memory_ordering_info.access_history[i].ordering = 0;
        memory_ordering_info.access_history[i].timestamp = 0;
        memory_ordering_info.access_history[i].cpu_id = 0;
        memory_ordering_info.access_history[i].operation = 0;
    }
}

u8 memory_ordering_is_supported(void) {
    return memory_ordering_info.memory_ordering_supported;
}

u8 memory_ordering_is_strong_ordering(void) {
    return memory_ordering_info.strong_ordering;
}

u8 memory_ordering_get_memory_model(void) {
    return memory_ordering_info.memory_model;
}

void memory_ordering_lfence(void) {
    u64 start_time = timer_get_ticks();
    
    __asm__ volatile("lfence" : : : "memory");
    
    u64 end_time = timer_get_ticks();
    u64 latency = end_time - start_time;
    
    if (latency > memory_ordering_info.max_fence_latency) {
        memory_ordering_info.max_fence_latency = latency;
    }
    if (latency < memory_ordering_info.min_fence_latency) {
        memory_ordering_info.min_fence_latency = latency;
    }
    
    memory_ordering_info.load_fences++;
    memory_ordering_record_fence(FENCE_TYPE_LOAD);
}

void memory_ordering_sfence(void) {
    u64 start_time = timer_get_ticks();
    
    __asm__ volatile("sfence" : : : "memory");
    
    u64 end_time = timer_get_ticks();
    u64 latency = end_time - start_time;
    
    if (latency > memory_ordering_info.max_fence_latency) {
        memory_ordering_info.max_fence_latency = latency;
    }
    if (latency < memory_ordering_info.min_fence_latency) {
        memory_ordering_info.min_fence_latency = latency;
    }
    
    memory_ordering_info.store_fences++;
    memory_ordering_record_fence(FENCE_TYPE_STORE);
}

void memory_ordering_mfence(void) {
    u64 start_time = timer_get_ticks();
    
    __asm__ volatile("mfence" : : : "memory");
    
    u64 end_time = timer_get_ticks();
    u64 latency = end_time - start_time;
    
    if (latency > memory_ordering_info.max_fence_latency) {
        memory_ordering_info.max_fence_latency = latency;
    }
    if (latency < memory_ordering_info.min_fence_latency) {
        memory_ordering_info.min_fence_latency = latency;
    }
    
    memory_ordering_info.full_fences++;
    memory_ordering_record_fence(FENCE_TYPE_FULL);
}

void memory_ordering_serialize(void) {
    u64 start_time = timer_get_ticks();
    
    /* CPUID acts as a serializing instruction */
    u32 eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0) : "memory");
    
    u64 end_time = timer_get_ticks();
    u64 latency = end_time - start_time;
    
    if (latency > memory_ordering_info.max_fence_latency) {
        memory_ordering_info.max_fence_latency = latency;
    }
    if (latency < memory_ordering_info.min_fence_latency) {
        memory_ordering_info.min_fence_latency = latency;
    }
    
    memory_ordering_info.serialize_fences++;
    memory_ordering_record_fence(FENCE_TYPE_SERIALIZE);
}

void memory_ordering_compiler_barrier(void) {
    __asm__ volatile("" : : : "memory");
}

u64 memory_ordering_atomic_load_acquire(volatile u64* address) {
    u64 value = *address;
    memory_ordering_lfence();
    
    /* Record access */
    memory_access_t* access = &memory_ordering_info.access_history[memory_ordering_info.access_history_index];
    access->address = (u64)address;
    access->value = value;
    access->ordering = MEMORY_ORDER_ACQUIRE;
    access->timestamp = timer_get_ticks();
    access->cpu_id = cpu_get_current_id();
    access->operation = 0;  /* load */
    
    memory_ordering_info.access_history_index = (memory_ordering_info.access_history_index + 1) % 128;
    
    return value;
}

void memory_ordering_atomic_store_release(volatile u64* address, u64 value) {
    memory_ordering_sfence();
    *address = value;
    
    /* Record access */
    memory_access_t* access = &memory_ordering_info.access_history[memory_ordering_info.access_history_index];
    access->address = (u64)address;
    access->value = value;
    access->ordering = MEMORY_ORDER_RELEASE;
    access->timestamp = timer_get_ticks();
    access->cpu_id = cpu_get_current_id();
    access->operation = 1;  /* store */
    
    memory_ordering_info.access_history_index = (memory_ordering_info.access_history_index + 1) % 128;
}

u64 memory_ordering_atomic_exchange(volatile u64* address, u64 new_value) {
    u64 old_value;
    __asm__ volatile("xchgq %0, %1" : "=r"(old_value), "+m"(*address) : "0"(new_value) : "memory");
    
    /* Record access */
    memory_access_t* access = &memory_ordering_info.access_history[memory_ordering_info.access_history_index];
    access->address = (u64)address;
    access->value = new_value;
    access->ordering = MEMORY_ORDER_SEQ_CST;
    access->timestamp = timer_get_ticks();
    access->cpu_id = cpu_get_current_id();
    access->operation = 1;  /* store */
    
    memory_ordering_info.access_history_index = (memory_ordering_info.access_history_index + 1) % 128;
    
    return old_value;
}

u8 memory_ordering_atomic_compare_exchange(volatile u64* address, u64* expected, u64 desired) {
    u64 old_value = *expected;
    u8 success;
    
    __asm__ volatile(
        "lock cmpxchgq %2, %1\n\t"
        "sete %0"
        : "=q"(success), "+m"(*address), "+a"(old_value)
        : "r"(desired)
        : "memory"
    );
    
    *expected = old_value;
    
    /* Record access */
    memory_access_t* access = &memory_ordering_info.access_history[memory_ordering_info.access_history_index];
    access->address = (u64)address;
    access->value = success ? desired : old_value;
    access->ordering = MEMORY_ORDER_SEQ_CST;
    access->timestamp = timer_get_ticks();
    access->cpu_id = cpu_get_current_id();
    access->operation = 1;  /* store */
    
    memory_ordering_info.access_history_index = (memory_ordering_info.access_history_index + 1) % 128;
    
    return success;
}

void memory_ordering_pause(void) {
    __asm__ volatile("pause" : : : "memory");
}

u32 memory_ordering_get_total_fences(void) {
    return memory_ordering_info.total_fences;
}

u32 memory_ordering_get_load_fences(void) {
    return memory_ordering_info.load_fences;
}

u32 memory_ordering_get_store_fences(void) {
    return memory_ordering_info.store_fences;
}

u32 memory_ordering_get_full_fences(void) {
    return memory_ordering_info.full_fences;
}

u32 memory_ordering_get_serialize_fences(void) {
    return memory_ordering_info.serialize_fences;
}

const fence_record_t* memory_ordering_get_fence_history(u32 index) {
    if (index >= 256) return 0;
    return &memory_ordering_info.fence_history[index];
}

const memory_access_t* memory_ordering_get_access_history(u32 index) {
    if (index >= 128) return 0;
    return &memory_ordering_info.access_history[index];
}

u32 memory_ordering_get_ordering_violations(void) {
    return memory_ordering_info.ordering_violations;
}

u64 memory_ordering_get_last_fence_time(void) {
    return memory_ordering_info.last_fence_time;
}

u64 memory_ordering_get_max_fence_latency(void) {
    return memory_ordering_info.max_fence_latency;
}

u64 memory_ordering_get_min_fence_latency(void) {
    return memory_ordering_info.min_fence_latency;
}

void memory_ordering_clear_statistics(void) {
    memory_ordering_info.total_fences = 0;
    memory_ordering_info.load_fences = 0;
    memory_ordering_info.store_fences = 0;
    memory_ordering_info.full_fences = 0;
    memory_ordering_info.serialize_fences = 0;
    memory_ordering_info.ordering_violations = 0;
    memory_ordering_info.max_fence_latency = 0;
    memory_ordering_info.min_fence_latency = 0xFFFFFFFFFFFFFFFF;
    memory_ordering_info.fence_history_index = 0;
    memory_ordering_info.access_history_index = 0;
}

const char* memory_ordering_get_fence_type_name(u32 fence_type) {
    switch (fence_type) {
        case FENCE_TYPE_LOAD: return "Load Fence";
        case FENCE_TYPE_STORE: return "Store Fence";
        case FENCE_TYPE_FULL: return "Full Fence";
        case FENCE_TYPE_SERIALIZE: return "Serialize";
        default: return "Unknown";
    }
}

const char* memory_ordering_get_memory_model_name(u8 model) {
    switch (model) {
        case MEMORY_MODEL_TSO: return "Total Store Order";
        case MEMORY_MODEL_PSO: return "Partial Store Order";
        case MEMORY_MODEL_RMO: return "Relaxed Memory Order";
        default: return "Unknown";
    }
}

void memory_ordering_detect_violation(u64 address1, u64 address2, u64 time1, u64 time2) {
    /* Simple violation detection - if two accesses to same address are out of order */
    if (address1 == address2 && time1 > time2) {
        memory_ordering_info.ordering_violations++;
    }
}

void memory_ordering_memory_barrier_full(void) {
    memory_ordering_mfence();
}

void memory_ordering_memory_barrier_read(void) {
    memory_ordering_lfence();
}

void memory_ordering_memory_barrier_write(void) {
    memory_ordering_sfence();
}
