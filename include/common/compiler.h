/*
 * Compiler-specific stuff
 */

#ifndef _BLOOD_COMPILER_H
#define _BLOOD_COMPILER_H

#define PACKED      __attribute__((packed))
#define ALIGN(x)    __attribute__((aligned(x)))
#define NORETURN    __attribute__((noreturn))
#define UNUSED      __attribute__((unused))

// Memory barriers
#define mb()        __asm__ volatile("mfence" ::: "memory")
#define rmb()       __asm__ volatile("lfence" ::: "memory")
#define wmb()       __asm__ volatile("sfence" ::: "memory")

#endif
