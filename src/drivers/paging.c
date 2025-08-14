/*
 * paging.c – x86 memory management with 4KB pages
 */

#include "kernel/types.h"

#define PAGE_SIZE 4096
#define PAGE_ENTRIES 1024
#define PAGE_DIR_SIZE (PAGE_ENTRIES * sizeof(u32))
#define PAGE_TABLE_SIZE (PAGE_ENTRIES * sizeof(u32))

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

/* Memory layout */
#define KERNEL_VIRTUAL_BASE 0xC0000000
#define KERNEL_PAGE_NUMBER  (KERNEL_VIRTUAL_BASE >> 22)

static u32* page_directory = 0;
static u32* page_tables = 0;
static u32 next_free_page = 0x400000; /* Start at 4MB */
static u32 total_memory = 0;

/* Bitmap for physical page allocation */
static u32* page_bitmap = 0;
static u32 bitmap_size = 0;

static inline void invlpg(u32 addr) {
    __asm__ volatile("invlpg (%0)" : : "r"(addr) : "memory");
}

static inline void load_page_directory(u32 pd_phys) {
    __asm__ volatile("mov %0, %%cr3" : : "r"(pd_phys));
}

static inline void enable_paging(void) {
    u32 cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000; /* Set PG bit */
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
}

static inline u32 get_cr2(void) {
    u32 addr;
    __asm__ volatile("mov %%cr2, %0" : "=r"(addr));
    return addr;
}

static u32 alloc_physical_page(void) {
    if (!page_bitmap) {
        /* Simple allocator before bitmap is set up */
        u32 page = next_free_page;
        next_free_page += PAGE_SIZE;
        return page;
    }
    
    /* Find free page in bitmap */
    for (u32 i = 0; i < bitmap_size; i++) {
        if (page_bitmap[i] != 0xFFFFFFFF) {
            for (u8 bit = 0; bit < 32; bit++) {
                if (!(page_bitmap[i] & (1 << bit))) {
                    page_bitmap[i] |= (1 << bit);
                    return (i * 32 + bit) * PAGE_SIZE;
                }
            }
        }
    }
    
    return 0; /* Out of memory */
}

static void free_physical_page(u32 addr) {
    if (!page_bitmap) return;
    
    u32 page_num = addr / PAGE_SIZE;
    u32 bitmap_index = page_num / 32;
    u8 bit_index = page_num % 32;
    
    if (bitmap_index < bitmap_size) {
        page_bitmap[bitmap_index] &= ~(1 << bit_index);
    }
}

void paging_init(u32 memory_size) {
    total_memory = memory_size;
    
    /* Allocate page directory */
    page_directory = (u32*)alloc_physical_page();
    
    /* Clear page directory */
    for (u32 i = 0; i < PAGE_ENTRIES; i++) {
        page_directory[i] = 0;
    }
    
    /* Identity map first 4MB (kernel space) */
    u32* first_page_table = (u32*)alloc_physical_page();
    page_directory[0] = (u32)first_page_table | PAGE_PRESENT | PAGE_WRITABLE;
    
    for (u32 i = 0; i < PAGE_ENTRIES; i++) {
        first_page_table[i] = (i * PAGE_SIZE) | PAGE_PRESENT | PAGE_WRITABLE;
    }
    
    /* Map kernel to higher half */
    page_directory[KERNEL_PAGE_NUMBER] = (u32)first_page_table | PAGE_PRESENT | PAGE_WRITABLE;
    
    /* Set up page bitmap */
    u32 total_pages = memory_size / PAGE_SIZE;
    bitmap_size = (total_pages + 31) / 32; /* Round up to nearest 32 */
    page_bitmap = (u32*)alloc_physical_page();
    
    /* Mark used pages in bitmap */
    for (u32 i = 0; i < bitmap_size; i++) {
        page_bitmap[i] = 0;
    }
    
    /* Mark kernel pages as used */
    u32 kernel_pages = next_free_page / PAGE_SIZE;
    for (u32 i = 0; i < kernel_pages; i++) {
        u32 bitmap_index = i / 32;
        u8 bit_index = i % 32;
        if (bitmap_index < bitmap_size) {
            page_bitmap[bitmap_index] |= (1 << bit_index);
        }
    }
    
    /* Load page directory and enable paging */
    load_page_directory((u32)page_directory);
    enable_paging();
}

u32 paging_map_page(u32 virtual_addr, u32 physical_addr, u32 flags) {
    u32 page_dir_index = virtual_addr >> 22;
    u32 page_table_index = (virtual_addr >> 12) & 0x3FF;
    
    /* Check if page table exists */
    if (!(page_directory[page_dir_index] & PAGE_PRESENT)) {
        /* Allocate new page table */
        u32* page_table = (u32*)alloc_physical_page();
        if (!page_table) return 0;
        
        /* Clear page table */
        for (u32 i = 0; i < PAGE_ENTRIES; i++) {
            page_table[i] = 0;
        }
        
        page_directory[page_dir_index] = (u32)page_table | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
    }
    
    /* Get page table */
    u32* page_table = (u32*)(page_directory[page_dir_index] & 0xFFFFF000);
    
    /* Map the page */
    page_table[page_table_index] = physical_addr | flags;
    
    /* Invalidate TLB entry */
    invlpg(virtual_addr);
    
    return 1;
}

void paging_unmap_page(u32 virtual_addr) {
    u32 page_dir_index = virtual_addr >> 22;
    u32 page_table_index = (virtual_addr >> 12) & 0x3FF;
    
    if (!(page_directory[page_dir_index] & PAGE_PRESENT)) {
        return; /* Page table doesn't exist */
    }
    
    u32* page_table = (u32*)(page_directory[page_dir_index] & 0xFFFFF000);
    
    if (page_table[page_table_index] & PAGE_PRESENT) {
        /* Free physical page */
        u32 physical_addr = page_table[page_table_index] & 0xFFFFF000;
        free_physical_page(physical_addr);
        
        /* Clear page table entry */
        page_table[page_table_index] = 0;
        
        /* Invalidate TLB entry */
        invlpg(virtual_addr);
    }
}

u32 paging_get_physical_addr(u32 virtual_addr) {
    u32 page_dir_index = virtual_addr >> 22;
    u32 page_table_index = (virtual_addr >> 12) & 0x3FF;
    u32 offset = virtual_addr & 0xFFF;
    
    if (!(page_directory[page_dir_index] & PAGE_PRESENT)) {
        return 0; /* Page not mapped */
    }
    
    u32* page_table = (u32*)(page_directory[page_dir_index] & 0xFFFFF000);
    
    if (!(page_table[page_table_index] & PAGE_PRESENT)) {
        return 0; /* Page not mapped */
    }
    
    return (page_table[page_table_index] & 0xFFFFF000) + offset;
}

void* paging_alloc_pages(u32 count) {
    u32 virtual_addr = 0xD0000000; /* Start of heap */
    
    /* Find free virtual address range */
    for (u32 i = 0; i < 0x10000000; i += PAGE_SIZE) {
        u8 found = 1;
        for (u32 j = 0; j < count; j++) {
            if (paging_get_physical_addr(virtual_addr + i + j * PAGE_SIZE)) {
                found = 0;
                break;
            }
        }
        
        if (found) {
            /* Allocate and map pages */
            for (u32 j = 0; j < count; j++) {
                u32 physical_addr = alloc_physical_page();
                if (!physical_addr) {
                    /* Clean up partial allocation */
                    for (u32 k = 0; k < j; k++) {
                        paging_unmap_page(virtual_addr + i + k * PAGE_SIZE);
                    }
                    return 0;
                }
                
                paging_map_page(virtual_addr + i + j * PAGE_SIZE, physical_addr,
                               PAGE_PRESENT | PAGE_WRITABLE);
            }
            
            return (void*)(virtual_addr + i);
        }
    }
    
    return 0; /* Out of virtual memory */
}

void paging_free_pages(void* ptr, u32 count) {
    u32 virtual_addr = (u32)ptr;
    
    for (u32 i = 0; i < count; i++) {
        paging_unmap_page(virtual_addr + i * PAGE_SIZE);
    }
}

void paging_handle_page_fault(u32 error_code, u32 virtual_addr) {
    extern void vga_printf(const char* fmt, ...);
    
    vga_printf("Page fault at %08X\n", virtual_addr);
    
    if (error_code & 1) {
        vga_printf("Protection violation\n");
    } else {
        vga_printf("Page not present\n");
        
        /* Try to handle demand paging */
        if (virtual_addr >= 0xD0000000 && virtual_addr < 0xE0000000) {
            /* Heap area - allocate page on demand */
            u32 physical_addr = alloc_physical_page();
            if (physical_addr) {
                u32 page_aligned = virtual_addr & 0xFFFFF000;
                paging_map_page(page_aligned, physical_addr,
                               PAGE_PRESENT | PAGE_WRITABLE);
                return; /* Page fault handled */
            }
        }
    }
    
    /* Unhandled page fault - system will halt */
}

u32 paging_get_free_memory(void) {
    if (!page_bitmap) return 0;
    
    u32 free_pages = 0;
    for (u32 i = 0; i < bitmap_size; i++) {
        for (u8 bit = 0; bit < 32; bit++) {
            if (!(page_bitmap[i] & (1 << bit))) {
                free_pages++;
            }
        }
    }
    
    return free_pages * PAGE_SIZE;
}

u32 paging_get_used_memory(void) {
    return total_memory - paging_get_free_memory();
}
