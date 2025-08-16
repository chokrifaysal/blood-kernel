/*
 * longmode.h – x86-64 long mode support and 64-bit extensions
 */

#ifndef LONGMODE_H
#define LONGMODE_H

#include "kernel/types.h"

/* Page table entry flags */
#define PTE_PRESENT             0x001
#define PTE_WRITABLE            0x002
#define PTE_USER                0x004
#define PTE_WRITETHROUGH        0x008
#define PTE_CACHEDISABLE        0x010
#define PTE_ACCESSED            0x020
#define PTE_DIRTY               0x040
#define PTE_PAGESIZE            0x080
#define PTE_GLOBAL              0x100
#define PTE_NX                  0x8000000000000000ULL

/* Core functions */
void longmode_init(void);

/* Status functions */
u8 longmode_is_supported(void);
u8 longmode_is_enabled(void);
u8 longmode_is_nx_supported(void);
u8 longmode_is_syscall_supported(void);
u8 longmode_is_pae_enabled(void);

/* Segment base management */
void longmode_set_fs_base(u64 base);
void longmode_set_gs_base(u64 base);
void longmode_set_kernel_gs_base(u64 base);
u64 longmode_get_fs_base(void);
u64 longmode_get_gs_base(void);
u64 longmode_get_kernel_gs_base(void);

/* System call setup */
void longmode_setup_syscall(u64 syscall_handler, u16 kernel_cs, u16 user_cs);

/* Page mapping */
u64 longmode_map_page(u64 virtual_addr, u64 physical_addr, u64 flags);
void longmode_unmap_page(u64 virtual_addr);

/* Security features */
void longmode_enable_smep(void);
void longmode_enable_smap(void);

/* Information functions */
u64 longmode_get_efer(void);

#endif
