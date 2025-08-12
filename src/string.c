/*
 * string.c - hand-rolled string ops
 */

#include "string.h"

void* memcpy(void* dst, const void* src, size_t n) {
    u8* d = dst;
    const u8* s = src;
    while (n--) *d++ = *s++;
    return dst;
}

void* memset(void* s, int c, size_t n) {
    u8* p = s;
    while (n--) *p++ = (u8)c;
    return s;
}

int memcmp(const void* a, const void* b, size_t n) {
    const u8* p1 = a, *p2 = b;
    while (n--) {
        if (*p1 != *p2) return *p1 < *p2 ? -1 : 1;
        p1++; p2++;
    }
    return 0;
}

size_t strlen(const char* s) {
    size_t len = 0;
    while (*s++) len++;
    return len;
}
