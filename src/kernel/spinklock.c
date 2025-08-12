/*
 * spinlock.c 
 */

#include "kernel/spinlock.h"
#include "common/compiler.h"

static inline void cpu_relax(void) {
#ifdef __x86_64__
    __asm__ volatile("pause");
#elif defined(__arm__)
    __asm__ volatile("yield");
#endif
}

void spin_lock(spinlock_t* lock) {
    while (__sync_lock_test_and_set(&lock->lock, 1)) {
        while (lock->lock) cpu_relax();
    }
}

void spin_unlock(spinlock_t* lock) {
    __sync_lock_release(&lock->lock);
}

void spin_lock_irq(spinlock_t* lock) {
#ifdef __x86_64__
    __asm__ volatile("cli");
#elif defined(__arm__)
    __asm__ volatile("cpsid i");
#endif
    spin_lock(lock);
}

void spin_unlock_irq(spinlock_t* lock) {
    spin_unlock(lock);
#ifdef __x86_64__
    __asm__ volatile("sti");
#elif defined(__arm__)
    __asm__ volatile("cpsie i");
#endif
}
