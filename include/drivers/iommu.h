/*
 * iommu.h – x86 Input/Output Memory Management Unit (Intel VT-d/AMD-Vi)
 */

#ifndef IOMMU_H
#define IOMMU_H

#include "kernel/types.h"

/* Core functions */
void iommu_init(void);
u8 iommu_is_enabled(void);

/* Unit management */
u8 iommu_add_unit(u32 base_address, u8 segment, u8 start_bus, u8 end_bus, u8 flags);
u8 iommu_get_unit_count(void);

/* Unit information */
u32 iommu_get_unit_capabilities(u8 unit_index);
u32 iommu_get_unit_extended_capabilities(u8 unit_index);
u8 iommu_get_unit_version_major(u8 unit_index);
u8 iommu_get_unit_version_minor(u8 unit_index);

/* Domain management */
u8 iommu_create_domain(u16 domain_id, u8 bus, u8 device, u8 function);

/* Page mapping */
u8 iommu_map_page(u16 domain_id, u64 iova, u64 physical_addr, u8 read, u8 write);
u8 iommu_unmap_page(u16 domain_id, u64 iova);

#endif
