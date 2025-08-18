/*
 * memory_ordering.h – x86 advanced memory ordering (memory barriers/fencing)
 */

#ifndef MEMORY_ORDERING_H
#define MEMORY_ORDERING_H

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

/* Core functions */
void memory_ordering_init(void);

/* Support detection */
u8 memory_ordering_is_supported(void);
u8 memory_ordering_is_strong_ordering(void);
u8 memory_ordering_get_memory_model(void);

/* Memory fences */
void memory_ordering_lfence(void);
void memory_ordering_sfence(void);
void memory_ordering_mfence(void);
void memory_ordering_serialize(void);
void memory_ordering_compiler_barrier(void);

/* Atomic operations with ordering */
u64 memory_ordering_atomic_load_acquire(volatile u64* address);
void memory_ordering_atomic_store_release(volatile u64* address, u64 value);
u64 memory_ordering_atomic_exchange(volatile u64* address, u64 new_value);
u8 memory_ordering_atomic_compare_exchange(volatile u64* address, u64* expected, u64 desired);

/* CPU synchronization */
void memory_ordering_pause(void);

/* Statistics */
u32 memory_ordering_get_total_fences(void);
u32 memory_ordering_get_load_fences(void);
u32 memory_ordering_get_store_fences(void);
u32 memory_ordering_get_full_fences(void);
u32 memory_ordering_get_serialize_fences(void);
const fence_record_t* memory_ordering_get_fence_history(u32 index);
const memory_access_t* memory_ordering_get_access_history(u32 index);
u32 memory_ordering_get_ordering_violations(void);
u64 memory_ordering_get_last_fence_time(void);
u64 memory_ordering_get_max_fence_latency(void);
u64 memory_ordering_get_min_fence_latency(void);

/* Convenience functions */
void memory_ordering_memory_barrier_full(void);
void memory_ordering_memory_barrier_read(void);
void memory_ordering_memory_barrier_write(void);

/* Violation detection */
void memory_ordering_detect_violation(u64 address1, u64 address2, u64 time1, u64 time2);

/* Utilities */
void memory_ordering_clear_statistics(void);
const char* memory_ordering_get_fence_type_name(u32 fence_type);
const char* memory_ordering_get_memory_model_name(u8 model);

#endif
