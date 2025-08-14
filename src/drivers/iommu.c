/*
 * iommu.c – x86 Input/Output Memory Management Unit (Intel VT-d/AMD-Vi)
 */

#include "kernel/types.h"

/* Intel VT-d registers */
#define VTD_VER_REG         0x00
#define VTD_CAP_REG         0x08
#define VTD_ECAP_REG        0x10
#define VTD_GCMD_REG        0x18
#define VTD_GSTS_REG        0x1C
#define VTD_RTADDR_REG      0x20
#define VTD_CCMD_REG        0x28
#define VTD_FSTS_REG        0x34
#define VTD_FECTL_REG       0x38
#define VTD_FEDATA_REG      0x3C
#define VTD_FEADDR_REG      0x40
#define VTD_FEUADDR_REG     0x44
#define VTD_AFLOG_REG       0x58
#define VTD_PMEN_REG        0x64
#define VTD_PLMBASE_REG     0x68
#define VTD_PLMLIMIT_REG    0x6C
#define VTD_PHMBASE_REG     0x70
#define VTD_PHMLIMIT_REG    0x78

/* Global command register bits */
#define VTD_GCMD_TE         0x80000000  /* Translation Enable */
#define VTD_GCMD_SRTP       0x40000000  /* Set Root Table Pointer */
#define VTD_GCMD_SFL        0x20000000  /* Set Fault Log */
#define VTD_GCMD_EAFL       0x10000000  /* Enable Advanced Fault Logging */
#define VTD_GCMD_WBF        0x08000000  /* Write Buffer Flush */
#define VTD_GCMD_QIE        0x04000000  /* Queued Invalidation Enable */
#define VTD_GCMD_IRE        0x02000000  /* Interrupt Remapping Enable */
#define VTD_GCMD_SIRTP      0x01000000  /* Set Interrupt Remap Table Pointer */
#define VTD_GCMD_CFI        0x00800000  /* Compatibility Format Interrupt */

/* Global status register bits */
#define VTD_GSTS_TES        0x80000000  /* Translation Enable Status */
#define VTD_GSTS_RTPS       0x40000000  /* Root Table Pointer Status */
#define VTD_GSTS_FLS        0x20000000  /* Fault Log Status */
#define VTD_GSTS_AFLS       0x10000000  /* Advanced Fault Log Status */
#define VTD_GSTS_WBFS       0x08000000  /* Write Buffer Flush Status */
#define VTD_GSTS_QIES       0x04000000  /* Queued Invalidation Enable Status */
#define VTD_GSTS_IRES       0x02000000  /* Interrupt Remapping Enable Status */
#define VTD_GSTS_IRTPS      0x01000000  /* Interrupt Remap Table Pointer Status */
#define VTD_GSTS_CFIS       0x00800000  /* Compatibility Format Interrupt Status */

/* Context command register bits */
#define VTD_CCMD_ICC        0x80000000  /* Invalidate Context Cache */
#define VTD_CCMD_CIRG_MASK  0x60000000  /* Context Invalidation Request Granularity */
#define VTD_CCMD_CIRG_GLOBAL 0x20000000 /* Global invalidation */
#define VTD_CCMD_CIRG_DOMAIN 0x40000000 /* Domain-selective invalidation */
#define VTD_CCMD_CIRG_DEVICE 0x60000000 /* Device-selective invalidation */

/* Root entry structure */
typedef struct {
    u64 present:1;
    u64 reserved1:11;
    u64 context_table_ptr:52;
    u64 reserved2;
} __attribute__((packed)) vtd_root_entry_t;

/* Context entry structure */
typedef struct {
    u64 present:1;
    u64 fault_processing_disable:1;
    u64 translation_type:2;
    u64 reserved1:8;
    u64 second_level_page_table_ptr:52;
    u64 address_width:3;
    u64 ignored:4;
    u64 reserved2:1;
    u64 domain_id:16;
    u64 reserved3:40;
} __attribute__((packed)) vtd_context_entry_t;

/* Page table entry structure */
typedef struct {
    u64 read:1;
    u64 write:1;
    u64 reserved1:5;
    u64 super_page:1;
    u64 reserved2:3;
    u64 snoop_behavior:1;
    u64 address:52;
} __attribute__((packed)) vtd_pte_t;

typedef struct {
    u32 base_address;
    u8 segment;
    u8 start_bus;
    u8 end_bus;
    u8 flags;
    vtd_root_entry_t* root_table;
    u8 enabled;
    u8 version_major;
    u8 version_minor;
    u32 capabilities;
    u32 extended_capabilities;
} iommu_unit_t;

static iommu_unit_t iommu_units[8];
static u8 iommu_unit_count = 0;
static u8 iommu_enabled = 0;

static inline u32 iommu_read32(u32 base, u32 offset) {
    return *(volatile u32*)(base + offset);
}

static inline void iommu_write32(u32 base, u32 offset, u32 value) {
    *(volatile u32*)(base + offset) = value;
}

static inline u64 iommu_read64(u32 base, u32 offset) {
    return *(volatile u64*)(base + offset);
}

static inline void iommu_write64(u32 base, u32 offset, u64 value) {
    *(volatile u64*)(base + offset) = value;
}

static u8 iommu_wait_operation(u32 base, u32 reg, u32 mask, u32 expected) {
    u32 timeout = 1000000;
    
    while (timeout--) {
        u32 status = iommu_read32(base, reg);
        if ((status & mask) == expected) {
            return 1;
        }
        __asm__ volatile("pause");
    }
    
    return 0;
}

static void iommu_invalidate_context_cache(iommu_unit_t* unit) {
    u32 ccmd = VTD_CCMD_ICC | VTD_CCMD_CIRG_GLOBAL;
    iommu_write32(unit->base_address, VTD_CCMD_REG, ccmd);
    
    iommu_wait_operation(unit->base_address, VTD_CCMD_REG, VTD_CCMD_ICC, 0);
}

static void iommu_setup_root_table(iommu_unit_t* unit) {
    extern void* paging_alloc_pages(u32 count);
    
    /* Allocate root table (4KB) */
    unit->root_table = (vtd_root_entry_t*)paging_alloc_pages(1);
    if (!unit->root_table) return;
    
    /* Clear root table */
    for (u32 i = 0; i < 256; i++) {
        unit->root_table[i].present = 0;
        unit->root_table[i].context_table_ptr = 0;
        unit->root_table[i].reserved2 = 0;
    }
    
    /* Set root table pointer */
    u64 rtaddr = (u64)unit->root_table;
    iommu_write64(unit->base_address, VTD_RTADDR_REG, rtaddr);
    
    /* Set root table pointer command */
    u32 gcmd = iommu_read32(unit->base_address, VTD_GCMD_REG);
    gcmd |= VTD_GCMD_SRTP;
    iommu_write32(unit->base_address, VTD_GCMD_REG, gcmd);
    
    /* Wait for completion */
    iommu_wait_operation(unit->base_address, VTD_GSTS_REG, VTD_GSTS_RTPS, VTD_GSTS_RTPS);
}

static void iommu_enable_translation(iommu_unit_t* unit) {
    /* Enable translation */
    u32 gcmd = iommu_read32(unit->base_address, VTD_GCMD_REG);
    gcmd |= VTD_GCMD_TE;
    iommu_write32(unit->base_address, VTD_GCMD_REG, gcmd);
    
    /* Wait for translation enable */
    iommu_wait_operation(unit->base_address, VTD_GSTS_REG, VTD_GSTS_TES, VTD_GSTS_TES);
    
    unit->enabled = 1;
}

u8 iommu_add_unit(u32 base_address, u8 segment, u8 start_bus, u8 end_bus, u8 flags) {
    if (iommu_unit_count >= 8) return 0;
    
    iommu_unit_t* unit = &iommu_units[iommu_unit_count];
    
    unit->base_address = base_address;
    unit->segment = segment;
    unit->start_bus = start_bus;
    unit->end_bus = end_bus;
    unit->flags = flags;
    unit->enabled = 0;
    
    /* Read version */
    u32 version = iommu_read32(base_address, VTD_VER_REG);
    unit->version_major = (version >> 4) & 0xF;
    unit->version_minor = version & 0xF;
    
    /* Read capabilities */
    unit->capabilities = iommu_read32(base_address, VTD_CAP_REG);
    unit->extended_capabilities = iommu_read32(base_address, VTD_ECAP_REG);
    
    iommu_unit_count++;
    return 1;
}

void iommu_init(void) {
    /* Initialize all IOMMU units */
    for (u8 i = 0; i < iommu_unit_count; i++) {
        iommu_unit_t* unit = &iommu_units[i];
        
        /* Setup root table */
        iommu_setup_root_table(unit);
        
        /* Invalidate context cache */
        iommu_invalidate_context_cache(unit);
        
        /* Enable translation */
        iommu_enable_translation(unit);
    }
    
    iommu_enabled = (iommu_unit_count > 0);
}

u8 iommu_create_domain(u16 domain_id, u8 bus, u8 device, u8 function) {
    if (!iommu_enabled) return 0;
    
    /* Find appropriate IOMMU unit */
    iommu_unit_t* unit = 0;
    for (u8 i = 0; i < iommu_unit_count; i++) {
        if (bus >= iommu_units[i].start_bus && bus <= iommu_units[i].end_bus) {
            unit = &iommu_units[i];
            break;
        }
    }
    
    if (!unit || !unit->root_table) return 0;
    
    /* Allocate context table if not present */
    if (!unit->root_table[bus].present) {
        extern void* paging_alloc_pages(u32 count);
        vtd_context_entry_t* context_table = (vtd_context_entry_t*)paging_alloc_pages(1);
        if (!context_table) return 0;
        
        /* Clear context table */
        for (u32 i = 0; i < 256; i++) {
            context_table[i].present = 0;
        }
        
        unit->root_table[bus].context_table_ptr = (u64)context_table >> 12;
        unit->root_table[bus].present = 1;
    }
    
    /* Get context table */
    vtd_context_entry_t* context_table = 
        (vtd_context_entry_t*)(unit->root_table[bus].context_table_ptr << 12);
    
    u8 devfn = (device << 3) | function;
    
    /* Allocate page table */
    extern void* paging_alloc_pages(u32 count);
    vtd_pte_t* page_table = (vtd_pte_t*)paging_alloc_pages(1);
    if (!page_table) return 0;
    
    /* Clear page table */
    for (u32 i = 0; i < 512; i++) {
        page_table[i].read = 0;
        page_table[i].write = 0;
        page_table[i].address = 0;
    }
    
    /* Setup context entry */
    context_table[devfn].present = 1;
    context_table[devfn].fault_processing_disable = 0;
    context_table[devfn].translation_type = 0; /* Untranslated requests */
    context_table[devfn].second_level_page_table_ptr = (u64)page_table >> 12;
    context_table[devfn].address_width = 2; /* 48-bit */
    context_table[devfn].domain_id = domain_id;
    
    /* Invalidate context cache */
    iommu_invalidate_context_cache(unit);
    
    return 1;
}

u8 iommu_map_page(u16 domain_id, u64 iova, u64 physical_addr, u8 read, u8 write) {
    return 1;
}

u8 iommu_unmap_page(u16 domain_id, u64 iova) {
    return 1;
}

u8 iommu_is_enabled(void) {
    return iommu_enabled;
}

u8 iommu_get_unit_count(void) {
    return iommu_unit_count;
}

u32 iommu_get_unit_capabilities(u8 unit_index) {
    if (unit_index >= iommu_unit_count) return 0;
    return iommu_units[unit_index].capabilities;
}

u32 iommu_get_unit_extended_capabilities(u8 unit_index) {
    if (unit_index >= iommu_unit_count) return 0;
    return iommu_units[unit_index].extended_capabilities;
}

u8 iommu_get_unit_version_major(u8 unit_index) {
    if (unit_index >= iommu_unit_count) return 0;
    return iommu_units[unit_index].version_major;
}

u8 iommu_get_unit_version_minor(u8 unit_index) {
    if (unit_index >= iommu_unit_count) return 0;
    return iommu_units[unit_index].version_minor;
}
