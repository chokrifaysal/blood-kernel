/*
 * atomic.h – x86 CPU synchronization primitives and atomic operations
 */

#ifndef ATOMIC_H
#define ATOMIC_H

#include "kernel/types.h"

/* Memory ordering constraints */
#define MEMORY_ORDER_RELAXED    0
#define MEMORY_ORDER_ACQUIRE    1
#define MEMORY_ORDER_RELEASE    2
#define MEMORY_ORDER_ACQ_REL    3
#define MEMORY_ORDER_SEQ_CST    4

/* Spinlock structure */
typedef struct {
    volatile u32 lock;
    u32 owner_cpu;
    u32 recursion_count;
} atomic_spinlock_t;

/* Core functions */
void atomic_init(void);

/* Support detection */
u8 atomic_is_supported(void);
u8 atomic_is_smp_system(void);
u32 atomic_get_cpu_count(void);
u32 atomic_get_cache_line_size(void);

/* Atomic load/store operations */
u32 atomic_load_u32(const volatile u32* ptr);
u64 atomic_load_u64(const volatile u64* ptr);
void atomic_store_u32(volatile u32* ptr, u32 value);
void atomic_store_u64(volatile u64* ptr, u64 value);

/* Atomic exchange operations */
u32 atomic_exchange_u32(volatile u32* ptr, u32 value);

/* Atomic compare-and-swap operations */
u8 atomic_compare_exchange_u32(volatile u32* ptr, u32* expected, u32 desired);
u8 atomic_compare_exchange_u64(volatile u64* ptr, u64* expected, u64 desired);

/* Atomic arithmetic operations */
u32 atomic_add_u32(volatile u32* ptr, u32 value);
u32 atomic_sub_u32(volatile u32* ptr, u32 value);
u32 atomic_inc_u32(volatile u32* ptr);
u32 atomic_dec_u32(volatile u32* ptr);

/* Atomic bitwise operations */
u32 atomic_and_u32(volatile u32* ptr, u32 value);
u32 atomic_or_u32(volatile u32* ptr, u32 value);
u32 atomic_xor_u32(volatile u32* ptr, u32 value);

/* Memory barriers */
void atomic_memory_barrier(void);
void atomic_read_barrier(void);
void atomic_write_barrier(void);
void atomic_compiler_barrier(void);

/* CPU synchronization primitives */
void atomic_pause(void);
void atomic_cpu_relax(void);

/* Spinlock operations */
void atomic_spinlock_init(atomic_spinlock_t* lock);
void atomic_spinlock_acquire(atomic_spinlock_t* lock);
u8 atomic_spinlock_try_acquire(atomic_spinlock_t* lock);
void atomic_spinlock_release(atomic_spinlock_t* lock);
u8 atomic_spinlock_is_locked(atomic_spinlock_t* lock);

/* Monitor/Mwait support */
void atomic_monitor(const volatile void* addr);
void atomic_mwait(u32 hints, u32 extensions);
void atomic_wait_for_change(const volatile u32* addr, u32 expected_value);

/* Cache line alignment helpers */
u8 atomic_is_cache_aligned(const void* ptr);
u32 atomic_align_to_cache_line(u32 size);

/* Test-and-set operations */
u8 atomic_test_and_set_bit(volatile u32* ptr, u32 bit);
u8 atomic_test_and_clear_bit(volatile u32* ptr, u32 bit);
u8 atomic_test_bit(const volatile u32* ptr, u32 bit);

#endif
