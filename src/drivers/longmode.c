/*
 * longmode.c – x86-64 long mode support and 64-bit extensions
 */

#include "kernel/types.h"

/* Long mode MSRs */
#define MSR_EFER                0xC0000080
#define MSR_STAR                0xC0000081
#define MSR_LSTAR               0xC0000082
#define MSR_CSTAR               0xC0000083
#define MSR_SYSCALL_MASK        0xC0000084
#define MSR_FS_BASE             0xC0000100
#define MSR_GS_BASE             0xC0000101
#define MSR_KERNEL_GS_BASE      0xC0000102

/* EFER bits */
#define EFER_SCE                0x01    /* System Call Extensions */
#define EFER_LME                0x100   /* Long Mode Enable */
#define EFER_LMA                0x400   /* Long Mode Active */
#define EFER_NXE                0x800   /* No-Execute Enable */
#define EFER_SVME               0x1000  /* Secure Virtual Machine Enable */
#define EFER_LMSLE              0x2000  /* Long Mode Segment Limit Enable */
#define EFER_FFXSR              0x4000  /* Fast FXSAVE/FXRSTOR */
#define EFER_TCE                0x8000  /* Translation Cache Extension */

/* CR4 bits for long mode */
#define CR4_PAE                 0x20    /* Physical Address Extension */
#define CR4_PGE                 0x80    /* Page Global Enable */
#define CR4_PCE                 0x100   /* Performance Counter Enable */
#define CR4_OSFXSR              0x200   /* OS FXSAVE/FXRSTOR Support */
#define CR4_OSXMMEXCPT          0x400   /* OS XMM Exception Support */
#define CR4_UMIP                0x800   /* User Mode Instruction Prevention */
#define CR4_VMXE                0x2000  /* VMX Enable */
#define CR4_SMXE                0x4000  /* SMX Enable */
#define CR4_FSGSBASE            0x10000 /* FS/GS Base Access Instructions */
#define CR4_PCIDE               0x20000 /* Process Context ID Enable */
#define CR4_OSXSAVE             0x40000 /* OS XSAVE Support */
#define CR4_SMEP                0x100000 /* Supervisor Mode Execution Prevention */
#define CR4_SMAP                0x200000 /* Supervisor Mode Access Prevention */

/* Page table entry bits for long mode */
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

typedef struct {
    u8 longmode_supported;
    u8 longmode_enabled;
    u8 nx_supported;
    u8 syscall_supported;
    u8 pae_enabled;
    u8 pge_enabled;
    u64 efer_value;
    u64* pml4_table;
    u64* pdpt_table;
    u64* pd_table;
    u64* pt_table;
    u64 fs_base;
    u64 gs_base;
    u64 kernel_gs_base;
} longmode_info_t;

static longmode_info_t longmode_info;

extern u64 msr_read(u32 msr);
extern void msr_write(u32 msr, u64 value);
extern u8 msr_is_supported(void);

static u8 longmode_check_support(void) {
    u32 eax, ebx, ecx, edx;
    
    /* Check for extended CPUID */
    __asm__ volatile("cpuid" : "=a"(eax) : "a"(0x80000000) : "ebx", "ecx", "edx");
    if (eax < 0x80000001) return 0;
    
    /* Check for long mode support */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x80000001));
    
    longmode_info.longmode_supported = (edx & (1 << 29)) != 0;
    longmode_info.nx_supported = (edx & (1 << 20)) != 0;
    longmode_info.syscall_supported = (edx & (1 << 11)) != 0;
    
    return longmode_info.longmode_supported;
}

static u8 longmode_setup_paging(void) {
    extern void* paging_alloc_pages(u32 count);
    
    /* Allocate page tables */
    longmode_info.pml4_table = (u64*)paging_alloc_pages(1);
    longmode_info.pdpt_table = (u64*)paging_alloc_pages(1);
    longmode_info.pd_table = (u64*)paging_alloc_pages(1);
    longmode_info.pt_table = (u64*)paging_alloc_pages(1);
    
    if (!longmode_info.pml4_table || !longmode_info.pdpt_table || 
        !longmode_info.pd_table || !longmode_info.pt_table) {
        return 0;
    }
    
    /* Clear page tables */
    for (u32 i = 0; i < 512; i++) {
        longmode_info.pml4_table[i] = 0;
        longmode_info.pdpt_table[i] = 0;
        longmode_info.pd_table[i] = 0;
        longmode_info.pt_table[i] = 0;
    }
    
    /* Setup PML4 entry */
    longmode_info.pml4_table[0] = (u64)longmode_info.pdpt_table | 
                                 PTE_PRESENT | PTE_WRITABLE | PTE_USER;
    
    /* Setup PDPT entry */
    longmode_info.pdpt_table[0] = (u64)longmode_info.pd_table | 
                                 PTE_PRESENT | PTE_WRITABLE | PTE_USER;
    
    /* Setup PD entries (2MB pages) */
    for (u32 i = 0; i < 512; i++) {
        longmode_info.pd_table[i] = (i * 0x200000) | 
                                   PTE_PRESENT | PTE_WRITABLE | PTE_PAGESIZE;
    }
    
    return 1;
}

static void longmode_enable_pae(void) {
    u32 cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= CR4_PAE | CR4_PGE | CR4_OSFXSR | CR4_OSXMMEXCPT;
    __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));
    
    longmode_info.pae_enabled = 1;
    longmode_info.pge_enabled = 1;
}

static void longmode_set_cr3(void) {
    u32 cr3 = (u32)longmode_info.pml4_table;
    __asm__ volatile("mov %0, %%cr3" : : "r"(cr3));
}

static void longmode_enable_long_mode(void) {
    u64 efer = msr_read(MSR_EFER);
    efer |= EFER_LME;
    
    if (longmode_info.nx_supported) {
        efer |= EFER_NXE;
    }
    
    if (longmode_info.syscall_supported) {
        efer |= EFER_SCE;
    }
    
    msr_write(MSR_EFER, efer);
    longmode_info.efer_value = efer;
}

static void longmode_enable_paging(void) {
    u32 cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000; /* Enable paging */
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
}

void longmode_init(void) {
    if (!msr_is_supported()) return;
    
    if (!longmode_check_support()) return;
    
    if (!longmode_setup_paging()) return;
    
    longmode_enable_pae();
    longmode_set_cr3();
    longmode_enable_long_mode();
    longmode_enable_paging();
    
    /* Check if long mode is active */
    u64 efer = msr_read(MSR_EFER);
    longmode_info.longmode_enabled = (efer & EFER_LMA) != 0;
    
    /* Initialize segment bases */
    longmode_info.fs_base = 0;
    longmode_info.gs_base = 0;
    longmode_info.kernel_gs_base = 0;
}

u8 longmode_is_supported(void) {
    return longmode_info.longmode_supported;
}

u8 longmode_is_enabled(void) {
    return longmode_info.longmode_enabled;
}

u8 longmode_is_nx_supported(void) {
    return longmode_info.nx_supported;
}

u8 longmode_is_syscall_supported(void) {
    return longmode_info.syscall_supported;
}

void longmode_set_fs_base(u64 base) {
    if (!longmode_info.longmode_enabled) return;
    
    msr_write(MSR_FS_BASE, base);
    longmode_info.fs_base = base;
}

void longmode_set_gs_base(u64 base) {
    if (!longmode_info.longmode_enabled) return;
    
    msr_write(MSR_GS_BASE, base);
    longmode_info.gs_base = base;
}

void longmode_set_kernel_gs_base(u64 base) {
    if (!longmode_info.longmode_enabled) return;
    
    msr_write(MSR_KERNEL_GS_BASE, base);
    longmode_info.kernel_gs_base = base;
}

u64 longmode_get_fs_base(void) {
    if (!longmode_info.longmode_enabled) return 0;
    
    return msr_read(MSR_FS_BASE);
}

u64 longmode_get_gs_base(void) {
    if (!longmode_info.longmode_enabled) return 0;
    
    return msr_read(MSR_GS_BASE);
}

u64 longmode_get_kernel_gs_base(void) {
    if (!longmode_info.longmode_enabled) return 0;
    
    return msr_read(MSR_KERNEL_GS_BASE);
}

void longmode_setup_syscall(u64 syscall_handler, u16 kernel_cs, u16 user_cs) {
    if (!longmode_info.syscall_supported) return;
    
    /* Setup STAR register */
    u64 star = ((u64)user_cs << 48) | ((u64)kernel_cs << 32);
    msr_write(MSR_STAR, star);
    
    /* Setup LSTAR register */
    msr_write(MSR_LSTAR, syscall_handler);
    
    /* Setup syscall mask */
    msr_write(MSR_SYSCALL_MASK, 0x200); /* Mask IF flag */
}

u64 longmode_map_page(u64 virtual_addr, u64 physical_addr, u64 flags) {
    if (!longmode_info.longmode_enabled) return 0;
    
    u64 pml4_index = (virtual_addr >> 39) & 0x1FF;
    u64 pdpt_index = (virtual_addr >> 30) & 0x1FF;
    u64 pd_index = (virtual_addr >> 21) & 0x1FF;
    u64 pt_index = (virtual_addr >> 12) & 0x1FF;
    
    /* For simplicity, use existing tables */
    if (pml4_index == 0 && pdpt_index == 0 && pd_index < 512) {
        /* Map 4KB page */
        if (!(longmode_info.pd_table[pd_index] & PTE_PAGESIZE)) {
            /* Need to allocate page table */
            extern void* paging_alloc_pages(u32 count);
            u64* pt = (u64*)paging_alloc_pages(1);
            if (!pt) return 0;
            
            for (u32 i = 0; i < 512; i++) {
                pt[i] = 0;
            }
            
            longmode_info.pd_table[pd_index] = (u64)pt | 
                                              PTE_PRESENT | PTE_WRITABLE | PTE_USER;
        }
        
        u64* pt = (u64*)(longmode_info.pd_table[pd_index] & ~0xFFF);
        pt[pt_index] = physical_addr | flags;
        
        /* Invalidate TLB */
        __asm__ volatile("invlpg (%0)" : : "r"(virtual_addr) : "memory");
        
        return virtual_addr;
    }
    
    return 0;
}

void longmode_unmap_page(u64 virtual_addr) {
    if (!longmode_info.longmode_enabled) return;
    
    u64 pml4_index = (virtual_addr >> 39) & 0x1FF;
    u64 pdpt_index = (virtual_addr >> 30) & 0x1FF;
    u64 pd_index = (virtual_addr >> 21) & 0x1FF;
    u64 pt_index = (virtual_addr >> 12) & 0x1FF;
    
    if (pml4_index == 0 && pdpt_index == 0 && pd_index < 512) {
        if (!(longmode_info.pd_table[pd_index] & PTE_PAGESIZE)) {
            u64* pt = (u64*)(longmode_info.pd_table[pd_index] & ~0xFFF);
            if (pt) {
                pt[pt_index] = 0;
                __asm__ volatile("invlpg (%0)" : : "r"(virtual_addr) : "memory");
            }
        }
    }
}

void longmode_enable_smep(void) {
    u32 eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    
    if (ebx & (1 << 7)) { /* SMEP support */
        u32 cr4;
        __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
        cr4 |= CR4_SMEP;
        __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));
    }
}

void longmode_enable_smap(void) {
    u32 eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    
    if (ebx & (1 << 20)) { /* SMAP support */
        u32 cr4;
        __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
        cr4 |= CR4_SMAP;
        __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));
    }
}

u64 longmode_get_efer(void) {
    return longmode_info.efer_value;
}

u8 longmode_is_pae_enabled(void) {
    return longmode_info.pae_enabled;
}
