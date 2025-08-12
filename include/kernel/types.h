/*
 * blood_kernel types.h
 * Basic types, no stdint.h dependency
 */

#ifndef _BLOOD_KERNEL_TYPES_H
#define _BLOOD_KERNEL_TYPES_H

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

typedef signed char        s8;
typedef signed short       s16;
typedef signed int         s32;
typedef signed long long   s64;

typedef u32 size_t;
typedef s32 ssize_t;

#ifndef NULL
#define NULL ((void*)0)
#endif

#define TRUE  1
#define FALSE 0
typedef int bool;

#endif
