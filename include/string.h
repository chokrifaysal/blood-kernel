/*
 * string.h - minimal memcpy/memset/memcmp for kernel use
 * No libc dependency, MISRA-friendly
 */

#ifndef _BLOOD_STRING_H
#define _BLOOD_STRING_H

#include "kernel/types.h"

void* memcpy(void* dst, const void* src, size_t n);
void* memset(void* s, int c, size_t n);
int   memcmp(const void* a, const void* b, size_t n);
size_t strlen(const char* s);

#endif
