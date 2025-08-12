/*
 * elf.c - static ELF loader for user tasks
 */

#include "kernel/elf.h"
#include "kernel/types.h"
#include "uart.h"
#include "string.h"   // we’ll roll our own

static void* memcpy(void* dst, const void* src, size_t n) {
    u8* d = dst;
    const u8* s = src;
    while (n--) *d++ = *s++;
    return dst;
}

static int memcmp(const void* a, const void* b, size_t n) {
    const u8* p1 = a, *p2 = b;
    while (n--) if (*p1++ != *p2++) return 1;
    return 0;
}

u32 elf_load(const u8* elf_data, u32* entry) {
    const elf_header_t* hdr = (const elf_header_t*)elf_data;
    
    if (memcmp(hdr->e_ident, "\x7F" "ELF", 4)) {
        uart_puts("Not ELF\r\n");
        return 0;
    }
    
    if (hdr->e_type != 2 || hdr->e_machine != 0x28) {
        uart_puts("Bad ELF type\r\n");
        return 0;
    }
    
    *entry = hdr->e_entry;
    
    const elf_phdr_t* phdr = (const elf_phdr_t*)(elf_data + hdr->e_phoff);
    
    for (u16 i = 0; i < hdr->e_phnum; i++) {
        if (phdr[i].p_type == 1) {   // PT_LOAD
            u8* dst = (u8*)phdr[i].p_vaddr;
            const u8* src = elf_data + phdr[i].p_offset;
            memcpy(dst, src, phdr[i].p_filesz);
            
            // zero .bss
            if (phdr[i].p_memsz > phdr[i].p_filesz) {
                memset(dst + phdr[i].p_filesz, 0,
                       phdr[i].p_memsz - phdr[i].p_filesz);
            }
        }
    }
    
    uart_puts("ELF loaded, entry=");
    uart_hex(*entry);
    uart_puts("\r\n");
    return 1;
}

static void* memset(void* s, int c, size_t n) {
    u8* p = s;
    while (n--) *p++ = (u8)c;
    return s;
}
