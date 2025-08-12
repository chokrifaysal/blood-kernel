/*
 * spinlock.h - bare-metal spinlocks
 */

#ifndef _BLOOD_SPINLOCK_H
#define _BLOOD_SPINLOCK_H

#include "kernel/types.h"

typedef struct {
    volatile u32 lock;
} spinlock_t;

void spin_lock(spinlock_t* lock);
void spin_unlock(spinlock_t* lock);
void spin_lock_irq(spinlock_t* lock);
void spin_unlock_irq(spinlock_t* lock);

#endif
