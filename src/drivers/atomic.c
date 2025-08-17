/*
 * atomic.c – x86 CPU synchronization primitives and atomic operations
 */

#include "kernel/types.h"

/* Memory ordering constraints */
#define MEMORY_ORDER_RELAXED    0
#define MEMORY_ORDER_ACQUIRE    1
#define MEMORY_ORDER_RELEASE    2
#define MEMORY_ORDER_ACQ_REL    3
#define MEMORY_ORDER_SEQ_CST    4

/* Lock prefix for SMP systems */
#define LOCK_PREFIX "lock; "

typedef struct {
    u8 atomic_supported;
    u8 cmpxchg_supported;
    u8 cmpxchg8b_supported;
    u8 cmpxchg16b_supported;
    u8 pause_supported;
    u8 monitor_mwait_supported;
    u8 smp_system;
    u32 cpu_count;
    u32 cache_line_size;
} atomic_info_t;

static atomic_info_t atomic_info;

static void atomic_detect_capabilities(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check for basic atomic support */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    
    atomic_info.cmpxchg_supported = (edx & (1 << 8)) != 0;  /* CX8 */
    atomic_info.cmpxchg8b_supported = (edx & (1 << 8)) != 0;
    atomic_info.cmpxchg16b_supported = (ecx & (1 << 13)) != 0;
    atomic_info.monitor_mwait_supported = (ecx & (1 << 3)) != 0;
    
    /* PAUSE instruction is available on all modern x86 CPUs */
    atomic_info.pause_supported = 1;
    
    /* Detect SMP system */
    extern u8 topology_get_num_logical_processors(void);
    atomic_info.cpu_count = topology_get_num_logical_processors();
    atomic_info.smp_system = atomic_info.cpu_count > 1;
    
    /* Get cache line size */
    atomic_info.cache_line_size = ((ebx >> 8) & 0xFF) * 8;
    if (atomic_info.cache_line_size == 0) {
        atomic_info.cache_line_size = 64; /* Default */
    }
    
    atomic_info.atomic_supported = 1;
}

void atomic_init(void) {
    atomic_info.atomic_supported = 0;
    atomic_info.cmpxchg_supported = 0;
    atomic_info.cmpxchg8b_supported = 0;
    atomic_info.cmpxchg16b_supported = 0;
    atomic_info.pause_supported = 0;
    atomic_info.monitor_mwait_supported = 0;
    atomic_info.smp_system = 0;
    atomic_info.cpu_count = 1;
    atomic_info.cache_line_size = 64;
    
    atomic_detect_capabilities();
}

u8 atomic_is_supported(void) {
    return atomic_info.atomic_supported;
}

u8 atomic_is_smp_system(void) {
    return atomic_info.smp_system;
}

u32 atomic_get_cpu_count(void) {
    return atomic_info.cpu_count;
}

u32 atomic_get_cache_line_size(void) {
    return atomic_info.cache_line_size;
}

/* Atomic load operations */
u32 atomic_load_u32(const volatile u32* ptr) {
    return *ptr; /* Naturally atomic on x86 for aligned 32-bit values */
}

u64 atomic_load_u64(const volatile u64* ptr) {
    if (atomic_info.cmpxchg8b_supported) {
        u64 result;
        __asm__ volatile(
            LOCK_PREFIX "cmpxchg8b %1"
            : "=A"(result)
            : "m"(*ptr), "a"(0), "d"(0), "b"(0), "c"(0)
            : "memory"
        );
        return result;
    } else {
        return *ptr; /* May not be atomic on 32-bit systems */
    }
}

/* Atomic store operations */
void atomic_store_u32(volatile u32* ptr, u32 value) {
    *ptr = value; /* Naturally atomic on x86 for aligned 32-bit values */
}

void atomic_store_u64(volatile u64* ptr, u64 value) {
    if (atomic_info.cmpxchg8b_supported) {
        u64 old_value;
        do {
            old_value = atomic_load_u64(ptr);
        } while (!atomic_compare_exchange_u64(ptr, &old_value, value));
    } else {
        *ptr = value; /* May not be atomic on 32-bit systems */
    }
}

/* Atomic exchange operations */
u32 atomic_exchange_u32(volatile u32* ptr, u32 value) {
    __asm__ volatile(
        LOCK_PREFIX "xchgl %0, %1"
        : "=r"(value), "+m"(*ptr)
        : "0"(value)
        : "memory"
    );
    return value;
}

/* Atomic compare-and-swap operations */
u8 atomic_compare_exchange_u32(volatile u32* ptr, u32* expected, u32 desired) {
    u32 old_value = *expected;
    u32 result;
    
    __asm__ volatile(
        LOCK_PREFIX "cmpxchgl %2, %1"
        : "=a"(result), "+m"(*ptr)
        : "r"(desired), "a"(old_value)
        : "memory"
    );
    
    *expected = result;
    return result == old_value;
}

u8 atomic_compare_exchange_u64(volatile u64* ptr, u64* expected, u64 desired) {
    if (!atomic_info.cmpxchg8b_supported) return 0;
    
    u32 desired_low = (u32)desired;
    u32 desired_high = (u32)(desired >> 32);
    u32 expected_low = (u32)*expected;
    u32 expected_high = (u32)(*expected >> 32);
    u8 success;
    
    __asm__ volatile(
        LOCK_PREFIX "cmpxchg8b %1\n\t"
        "setz %0"
        : "=q"(success), "+m"(*ptr), "=a"(expected_low), "=d"(expected_high)
        : "a"(expected_low), "d"(expected_high), "b"(desired_low), "c"(desired_high)
        : "memory"
    );
    
    *expected = ((u64)expected_high << 32) | expected_low;
    return success;
}

/* Atomic arithmetic operations */
u32 atomic_add_u32(volatile u32* ptr, u32 value) {
    __asm__ volatile(
        LOCK_PREFIX "addl %1, %0"
        : "+m"(*ptr)
        : "r"(value)
        : "memory"
    );
    return *ptr;
}

u32 atomic_sub_u32(volatile u32* ptr, u32 value) {
    __asm__ volatile(
        LOCK_PREFIX "subl %1, %0"
        : "+m"(*ptr)
        : "r"(value)
        : "memory"
    );
    return *ptr;
}

u32 atomic_inc_u32(volatile u32* ptr) {
    __asm__ volatile(
        LOCK_PREFIX "incl %0"
        : "+m"(*ptr)
        :
        : "memory"
    );
    return *ptr;
}

u32 atomic_dec_u32(volatile u32* ptr) {
    __asm__ volatile(
        LOCK_PREFIX "decl %0"
        : "+m"(*ptr)
        :
        : "memory"
    );
    return *ptr;
}

/* Atomic bitwise operations */
u32 atomic_and_u32(volatile u32* ptr, u32 value) {
    __asm__ volatile(
        LOCK_PREFIX "andl %1, %0"
        : "+m"(*ptr)
        : "r"(value)
        : "memory"
    );
    return *ptr;
}

u32 atomic_or_u32(volatile u32* ptr, u32 value) {
    __asm__ volatile(
        LOCK_PREFIX "orl %1, %0"
        : "+m"(*ptr)
        : "r"(value)
        : "memory"
    );
    return *ptr;
}

u32 atomic_xor_u32(volatile u32* ptr, u32 value) {
    __asm__ volatile(
        LOCK_PREFIX "xorl %1, %0"
        : "+m"(*ptr)
        : "r"(value)
        : "memory"
    );
    return *ptr;
}

/* Memory barriers */
void atomic_memory_barrier(void) {
    __asm__ volatile("mfence" : : : "memory");
}

void atomic_read_barrier(void) {
    __asm__ volatile("lfence" : : : "memory");
}

void atomic_write_barrier(void) {
    __asm__ volatile("sfence" : : : "memory");
}

void atomic_compiler_barrier(void) {
    __asm__ volatile("" : : : "memory");
}

/* CPU synchronization primitives */
void atomic_pause(void) {
    if (atomic_info.pause_supported) {
        __asm__ volatile("pause" : : : "memory");
    } else {
        __asm__ volatile("rep; nop" : : : "memory");
    }
}

void atomic_cpu_relax(void) {
    atomic_pause();
}

/* Spinlock implementation */
typedef struct {
    volatile u32 lock;
    u32 owner_cpu;
    u32 recursion_count;
} atomic_spinlock_t;

void atomic_spinlock_init(atomic_spinlock_t* lock) {
    lock->lock = 0;
    lock->owner_cpu = 0xFFFFFFFF;
    lock->recursion_count = 0;
}

void atomic_spinlock_acquire(atomic_spinlock_t* lock) {
    while (1) {
        /* Try to acquire the lock */
        if (atomic_compare_exchange_u32(&lock->lock, &(u32){0}, 1)) {
            atomic_memory_barrier();
            return;
        }
        
        /* Spin with pause to reduce bus traffic */
        while (atomic_load_u32(&lock->lock) != 0) {
            atomic_pause();
        }
    }
}

u8 atomic_spinlock_try_acquire(atomic_spinlock_t* lock) {
    u32 expected = 0;
    if (atomic_compare_exchange_u32(&lock->lock, &expected, 1)) {
        atomic_memory_barrier();
        return 1;
    }
    return 0;
}

void atomic_spinlock_release(atomic_spinlock_t* lock) {
    atomic_memory_barrier();
    atomic_store_u32(&lock->lock, 0);
}

u8 atomic_spinlock_is_locked(atomic_spinlock_t* lock) {
    return atomic_load_u32(&lock->lock) != 0;
}

/* Monitor/Mwait support */
void atomic_monitor(const volatile void* addr) {
    if (!atomic_info.monitor_mwait_supported) return;
    
    __asm__ volatile(
        "monitor"
        :
        : "a"(addr), "c"(0), "d"(0)
    );
}

void atomic_mwait(u32 hints, u32 extensions) {
    if (!atomic_info.monitor_mwait_supported) return;
    
    __asm__ volatile(
        "mwait"
        :
        : "a"(hints), "c"(extensions)
    );
}

/* Wait for memory location to change */
void atomic_wait_for_change(const volatile u32* addr, u32 expected_value) {
    if (atomic_info.monitor_mwait_supported) {
        while (atomic_load_u32(addr) == expected_value) {
            atomic_monitor(addr);
            if (atomic_load_u32(addr) == expected_value) {
                atomic_mwait(0, 0);
            }
        }
    } else {
        while (atomic_load_u32(addr) == expected_value) {
            atomic_pause();
        }
    }
}

/* Cache line alignment helpers */
u8 atomic_is_cache_aligned(const void* ptr) {
    return ((u32)ptr & (atomic_info.cache_line_size - 1)) == 0;
}

u32 atomic_align_to_cache_line(u32 size) {
    return (size + atomic_info.cache_line_size - 1) & ~(atomic_info.cache_line_size - 1);
}

/* Test-and-set operations */
u8 atomic_test_and_set_bit(volatile u32* ptr, u32 bit) {
    u8 result;
    __asm__ volatile(
        LOCK_PREFIX "btsl %2, %1\n\t"
        "setc %0"
        : "=q"(result), "+m"(*ptr)
        : "r"(bit)
        : "memory"
    );
    return result;
}

u8 atomic_test_and_clear_bit(volatile u32* ptr, u32 bit) {
    u8 result;
    __asm__ volatile(
        LOCK_PREFIX "btrl %2, %1\n\t"
        "setc %0"
        : "=q"(result), "+m"(*ptr)
        : "r"(bit)
        : "memory"
    );
    return result;
}

u8 atomic_test_bit(const volatile u32* ptr, u32 bit) {
    u8 result;
    __asm__ volatile(
        "btl %2, %1\n\t"
        "setc %0"
        : "=q"(result)
        : "m"(*ptr), "r"(bit)
        : "memory"
    );
    return result;
}
