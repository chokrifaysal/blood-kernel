/*
 * paging.h – x86 memory management with paging
 */

#ifndef PAGING_H
#define PAGING_H

#include "kernel/types.h"

/* Page flags */
#define PAGE_PRESENT    0x001
#define PAGE_WRITABLE   0x002
#define PAGE_USER       0x004
#define PAGE_WRITETHROUGH 0x008
#define PAGE_CACHE_DISABLE 0x010
#define PAGE_ACCESSED   0x020
#define PAGE_DIRTY      0x040
#define PAGE_SIZE_4MB   0x080
#define PAGE_GLOBAL     0x100

/* Core functions */
void paging_init(u32 memory_size);

/* Page mapping */
u32 paging_map_page(u32 virtual_addr, u32 physical_addr, u32 flags);
void paging_unmap_page(u32 virtual_addr);
u32 paging_get_physical_addr(u32 virtual_addr);

/* Memory allocation */
void* paging_alloc_pages(u32 count);
void paging_free_pages(void* ptr, u32 count);

/* Fault handling */
void paging_handle_page_fault(u32 error_code, u32 virtual_addr);

/* Statistics */
u32 paging_get_free_memory(void);
u32 paging_get_used_memory(void);

#endif
