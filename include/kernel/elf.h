/*
 * elf.h - minimal ELF parser for static tasks
 */

#ifndef _BLOOD_ELF_H
#define _BLOOD_ELF_H

#include "kernel/types.h"

#define EI_NIDENT 16

typedef struct {
    u8  e_ident[EI_NIDENT];
    u16 e_type;
    u16 e_machine;
    u32 e_version;
    u32 e_entry;
    u32 e_phoff;
    u32 e_shoff;
    u32 e_flags;
    u16 e_ehsize;
    u16 e_phentsize;
    u16 e_phnum;
    u16 e_shentsize;
    u16 e_shnum;
    u16 e_shstrndx;
} elf_header_t;

typedef struct {
    u32 p_type;
    u32 p_offset;
    u32 p_vaddr;
    u32 p_paddr;
    u32 p_filesz;
    u32 p_memsz;
    u32 p_flags;
    u32 p_align;
} elf_phdr_t;

u32 elf_load(const u8* elf_data, u32* entry);

#endif
