/*
 * interrupt_routing.c – x86 advanced interrupt routing (I/O APIC/MSI/MSI-X)
 */

#include "kernel/types.h"

/* I/O APIC registers */
#define IOAPIC_REGSEL                   0x00
#define IOAPIC_REGWIN                   0x10

/* I/O APIC register indices */
#define IOAPIC_ID                       0x00
#define IOAPIC_VER                      0x01
#define IOAPIC_ARB                      0x02
#define IOAPIC_REDTBL_BASE              0x10

/* MSI capability structure */
#define MSI_CAP_ID                      0x05
#define MSIX_CAP_ID                     0x11

/* MSI control register bits */
#define MSI_CTRL_ENABLE                 (1 << 0)
#define MSI_CTRL_MULTI_CAP_MASK         (0x7 << 1)
#define MSI_CTRL_MULTI_EN_MASK          (0x7 << 4)
#define MSI_CTRL_64BIT                  (1 << 7)
#define MSI_CTRL_PER_VECTOR_MASK        (1 << 8)

/* MSI-X control register bits */
#define MSIX_CTRL_ENABLE                (1 << 15)
#define MSIX_CTRL_FUNCTION_MASK         (1 << 14)
#define MSIX_CTRL_TABLE_SIZE_MASK       0x7FF

/* Interrupt delivery modes */
#define DELIVERY_MODE_FIXED             0x0
#define DELIVERY_MODE_LOWEST            0x1
#define DELIVERY_MODE_SMI               0x2
#define DELIVERY_MODE_NMI               0x4
#define DELIVERY_MODE_INIT              0x5
#define DELIVERY_MODE_EXTINT            0x7

/* Interrupt trigger modes */
#define TRIGGER_MODE_EDGE               0
#define TRIGGER_MODE_LEVEL              1

/* Interrupt polarity */
#define POLARITY_ACTIVE_HIGH            0
#define POLARITY_ACTIVE_LOW             1

typedef struct {
    u64 address;
    u32 data;
    u8 vector;
    u8 delivery_mode;
    u8 destination;
    u8 enabled;
} msi_entry_t;

typedef struct {
    u64 address;
    u32 data;
    u32 vector_control;
    u8 masked;
} msix_entry_t;

typedef struct {
    u32 vector;
    u8 delivery_mode;
    u8 dest_mode;
    u8 polarity;
    u8 trigger_mode;
    u8 mask;
    u8 destination;
} ioapic_entry_t;

typedef struct {
    u32 base_address;
    u8 id;
    u8 version;
    u8 max_redirection_entries;
    ioapic_entry_t entries[24];
    u32 interrupt_count[24];
} ioapic_info_t;

typedef struct {
    u8 interrupt_routing_supported;
    u8 ioapic_supported;
    u8 msi_supported;
    u8 msix_supported;
    u8 num_ioapics;
    ioapic_info_t ioapics[8];
    msi_entry_t msi_entries[256];
    msix_entry_t msix_entries[2048];
    u32 num_msi_entries;
    u32 num_msix_entries;
    u32 total_interrupts_routed;
    u32 msi_interrupts;
    u32 msix_interrupts;
    u32 ioapic_interrupts;
    u64 last_interrupt_time;
} interrupt_routing_info_t;

static interrupt_routing_info_t interrupt_routing_info;

extern u32 inl(u16 port);
extern void outl(u16 port, u32 value);
extern u32 pci_config_read(u8 bus, u8 device, u8 function, u8 offset);
extern void pci_config_write(u8 bus, u8 device, u8 function, u8 offset, u32 value);
extern u64 timer_get_ticks(void);

static u32 ioapic_read_reg(u32 ioapic_base, u8 reg) {
    volatile u32* ioapic = (volatile u32*)ioapic_base;
    ioapic[IOAPIC_REGSEL] = reg;
    return ioapic[IOAPIC_REGWIN];
}

static void ioapic_write_reg(u32 ioapic_base, u8 reg, u32 value) {
    volatile u32* ioapic = (volatile u32*)ioapic_base;
    ioapic[IOAPIC_REGSEL] = reg;
    ioapic[IOAPIC_REGWIN] = value;
}

static void interrupt_routing_detect_ioapic(void) {
    /* Assume standard I/O APIC base address */
    u32 ioapic_base = 0xFEC00000;
    
    /* Try to read I/O APIC ID register */
    u32 id_reg = ioapic_read_reg(ioapic_base, IOAPIC_ID);
    if (id_reg != 0xFFFFFFFF) {
        interrupt_routing_info.ioapic_supported = 1;
        interrupt_routing_info.num_ioapics = 1;
        
        ioapic_info_t* ioapic = &interrupt_routing_info.ioapics[0];
        ioapic->base_address = ioapic_base;
        ioapic->id = (id_reg >> 24) & 0xF;
        
        /* Read version register */
        u32 ver_reg = ioapic_read_reg(ioapic_base, IOAPIC_VER);
        ioapic->version = ver_reg & 0xFF;
        ioapic->max_redirection_entries = ((ver_reg >> 16) & 0xFF) + 1;
        
        /* Initialize redirection entries */
        for (u8 i = 0; i < ioapic->max_redirection_entries && i < 24; i++) {
            u32 low = ioapic_read_reg(ioapic_base, IOAPIC_REDTBL_BASE + (i * 2));
            u32 high = ioapic_read_reg(ioapic_base, IOAPIC_REDTBL_BASE + (i * 2) + 1);
            
            ioapic->entries[i].vector = low & 0xFF;
            ioapic->entries[i].delivery_mode = (low >> 8) & 0x7;
            ioapic->entries[i].dest_mode = (low >> 11) & 0x1;
            ioapic->entries[i].polarity = (low >> 13) & 0x1;
            ioapic->entries[i].trigger_mode = (low >> 15) & 0x1;
            ioapic->entries[i].mask = (low >> 16) & 0x1;
            ioapic->entries[i].destination = (high >> 24) & 0xFF;
            ioapic->interrupt_count[i] = 0;
        }
    }
}

static void interrupt_routing_detect_msi(void) {
    /* Scan PCI devices for MSI capability */
    for (u8 bus = 0; bus < 8; bus++) {
        for (u8 device = 0; device < 32; device++) {
            for (u8 function = 0; function < 8; function++) {
                u32 vendor_device = pci_config_read(bus, device, function, 0);
                if (vendor_device == 0xFFFFFFFF) continue;
                
                /* Check for MSI capability */
                u32 cap_ptr = pci_config_read(bus, device, function, 0x34) & 0xFF;
                while (cap_ptr != 0) {
                    u32 cap_header = pci_config_read(bus, device, function, cap_ptr);
                    u8 cap_id = cap_header & 0xFF;
                    
                    if (cap_id == MSI_CAP_ID) {
                        interrupt_routing_info.msi_supported = 1;
                        break;
                    } else if (cap_id == MSIX_CAP_ID) {
                        interrupt_routing_info.msix_supported = 1;
                        break;
                    }
                    
                    cap_ptr = (cap_header >> 8) & 0xFF;
                }
            }
        }
    }
}

void interrupt_routing_init(void) {
    interrupt_routing_info.interrupt_routing_supported = 0;
    interrupt_routing_info.ioapic_supported = 0;
    interrupt_routing_info.msi_supported = 0;
    interrupt_routing_info.msix_supported = 0;
    interrupt_routing_info.num_ioapics = 0;
    interrupt_routing_info.num_msi_entries = 0;
    interrupt_routing_info.num_msix_entries = 0;
    interrupt_routing_info.total_interrupts_routed = 0;
    interrupt_routing_info.msi_interrupts = 0;
    interrupt_routing_info.msix_interrupts = 0;
    interrupt_routing_info.ioapic_interrupts = 0;
    interrupt_routing_info.last_interrupt_time = 0;
    
    for (u8 i = 0; i < 8; i++) {
        interrupt_routing_info.ioapics[i].base_address = 0;
        interrupt_routing_info.ioapics[i].id = 0;
        interrupt_routing_info.ioapics[i].version = 0;
        interrupt_routing_info.ioapics[i].max_redirection_entries = 0;
        
        for (u8 j = 0; j < 24; j++) {
            interrupt_routing_info.ioapics[i].entries[j].vector = 0;
            interrupt_routing_info.ioapics[i].entries[j].delivery_mode = 0;
            interrupt_routing_info.ioapics[i].entries[j].dest_mode = 0;
            interrupt_routing_info.ioapics[i].entries[j].polarity = 0;
            interrupt_routing_info.ioapics[i].entries[j].trigger_mode = 0;
            interrupt_routing_info.ioapics[i].entries[j].mask = 1;
            interrupt_routing_info.ioapics[i].entries[j].destination = 0;
            interrupt_routing_info.ioapics[i].interrupt_count[j] = 0;
        }
    }
    
    for (u32 i = 0; i < 256; i++) {
        interrupt_routing_info.msi_entries[i].address = 0;
        interrupt_routing_info.msi_entries[i].data = 0;
        interrupt_routing_info.msi_entries[i].vector = 0;
        interrupt_routing_info.msi_entries[i].delivery_mode = 0;
        interrupt_routing_info.msi_entries[i].destination = 0;
        interrupt_routing_info.msi_entries[i].enabled = 0;
    }
    
    for (u32 i = 0; i < 2048; i++) {
        interrupt_routing_info.msix_entries[i].address = 0;
        interrupt_routing_info.msix_entries[i].data = 0;
        interrupt_routing_info.msix_entries[i].vector_control = 0;
        interrupt_routing_info.msix_entries[i].masked = 1;
    }
    
    interrupt_routing_detect_ioapic();
    interrupt_routing_detect_msi();
    
    if (interrupt_routing_info.ioapic_supported || 
        interrupt_routing_info.msi_supported || 
        interrupt_routing_info.msix_supported) {
        interrupt_routing_info.interrupt_routing_supported = 1;
    }
}

u8 interrupt_routing_is_supported(void) {
    return interrupt_routing_info.interrupt_routing_supported;
}

u8 interrupt_routing_is_ioapic_supported(void) {
    return interrupt_routing_info.ioapic_supported;
}

u8 interrupt_routing_is_msi_supported(void) {
    return interrupt_routing_info.msi_supported;
}

u8 interrupt_routing_is_msix_supported(void) {
    return interrupt_routing_info.msix_supported;
}

u8 interrupt_routing_get_num_ioapics(void) {
    return interrupt_routing_info.num_ioapics;
}

const ioapic_info_t* interrupt_routing_get_ioapic_info(u8 ioapic_index) {
    if (ioapic_index >= interrupt_routing_info.num_ioapics) return 0;
    return &interrupt_routing_info.ioapics[ioapic_index];
}

u8 interrupt_routing_setup_ioapic_entry(u8 ioapic_index, u8 pin, u8 vector, 
                                       u8 delivery_mode, u8 dest_mode, 
                                       u8 polarity, u8 trigger_mode, u8 destination) {
    if (ioapic_index >= interrupt_routing_info.num_ioapics) return 0;
    if (pin >= interrupt_routing_info.ioapics[ioapic_index].max_redirection_entries) return 0;
    
    ioapic_info_t* ioapic = &interrupt_routing_info.ioapics[ioapic_index];
    
    /* Configure redirection table entry */
    u32 low = vector | (delivery_mode << 8) | (dest_mode << 11) | 
              (polarity << 13) | (trigger_mode << 15);
    u32 high = (destination << 24);
    
    ioapic_write_reg(ioapic->base_address, IOAPIC_REDTBL_BASE + (pin * 2), low);
    ioapic_write_reg(ioapic->base_address, IOAPIC_REDTBL_BASE + (pin * 2) + 1, high);
    
    /* Update local copy */
    ioapic->entries[pin].vector = vector;
    ioapic->entries[pin].delivery_mode = delivery_mode;
    ioapic->entries[pin].dest_mode = dest_mode;
    ioapic->entries[pin].polarity = polarity;
    ioapic->entries[pin].trigger_mode = trigger_mode;
    ioapic->entries[pin].mask = 0;
    ioapic->entries[pin].destination = destination;
    
    interrupt_routing_info.total_interrupts_routed++;
    return 1;
}

u8 interrupt_routing_mask_ioapic_entry(u8 ioapic_index, u8 pin) {
    if (ioapic_index >= interrupt_routing_info.num_ioapics) return 0;
    if (pin >= interrupt_routing_info.ioapics[ioapic_index].max_redirection_entries) return 0;
    
    ioapic_info_t* ioapic = &interrupt_routing_info.ioapics[ioapic_index];
    
    u32 low = ioapic_read_reg(ioapic->base_address, IOAPIC_REDTBL_BASE + (pin * 2));
    low |= (1 << 16);  /* Set mask bit */
    ioapic_write_reg(ioapic->base_address, IOAPIC_REDTBL_BASE + (pin * 2), low);
    
    ioapic->entries[pin].mask = 1;
    return 1;
}

u8 interrupt_routing_unmask_ioapic_entry(u8 ioapic_index, u8 pin) {
    if (ioapic_index >= interrupt_routing_info.num_ioapics) return 0;
    if (pin >= interrupt_routing_info.ioapics[ioapic_index].max_redirection_entries) return 0;
    
    ioapic_info_t* ioapic = &interrupt_routing_info.ioapics[ioapic_index];
    
    u32 low = ioapic_read_reg(ioapic->base_address, IOAPIC_REDTBL_BASE + (pin * 2));
    low &= ~(1 << 16);  /* Clear mask bit */
    ioapic_write_reg(ioapic->base_address, IOAPIC_REDTBL_BASE + (pin * 2), low);
    
    ioapic->entries[pin].mask = 0;
    return 1;
}

u8 interrupt_routing_setup_msi(u8 bus, u8 device, u8 function, u8 vector, u8 destination) {
    if (!interrupt_routing_info.msi_supported) return 0;
    if (interrupt_routing_info.num_msi_entries >= 256) return 0;
    
    /* Find MSI capability */
    u32 cap_ptr = pci_config_read(bus, device, function, 0x34) & 0xFF;
    while (cap_ptr != 0) {
        u32 cap_header = pci_config_read(bus, device, function, cap_ptr);
        if ((cap_header & 0xFF) == MSI_CAP_ID) break;
        cap_ptr = (cap_header >> 8) & 0xFF;
    }
    
    if (cap_ptr == 0) return 0;
    
    /* Configure MSI */
    u64 msi_address = 0xFEE00000 | (destination << 12);
    u32 msi_data = vector | (DELIVERY_MODE_FIXED << 8);
    
    /* Write MSI address and data */
    pci_config_write(bus, device, function, cap_ptr + 4, (u32)msi_address);
    pci_config_write(bus, device, function, cap_ptr + 8, msi_data);
    
    /* Enable MSI */
    u32 control = pci_config_read(bus, device, function, cap_ptr + 2);
    control |= MSI_CTRL_ENABLE;
    pci_config_write(bus, device, function, cap_ptr + 2, control);
    
    /* Record MSI entry */
    msi_entry_t* entry = &interrupt_routing_info.msi_entries[interrupt_routing_info.num_msi_entries];
    entry->address = msi_address;
    entry->data = msi_data;
    entry->vector = vector;
    entry->delivery_mode = DELIVERY_MODE_FIXED;
    entry->destination = destination;
    entry->enabled = 1;
    
    interrupt_routing_info.num_msi_entries++;
    interrupt_routing_info.total_interrupts_routed++;
    return 1;
}

void interrupt_routing_handle_interrupt(u8 vector) {
    interrupt_routing_info.last_interrupt_time = timer_get_ticks();
    
    /* Update interrupt counters */
    for (u8 i = 0; i < interrupt_routing_info.num_ioapics; i++) {
        ioapic_info_t* ioapic = &interrupt_routing_info.ioapics[i];
        for (u8 j = 0; j < ioapic->max_redirection_entries && j < 24; j++) {
            if (ioapic->entries[j].vector == vector && !ioapic->entries[j].mask) {
                ioapic->interrupt_count[j]++;
                interrupt_routing_info.ioapic_interrupts++;
                return;
            }
        }
    }
    
    /* Check MSI entries */
    for (u32 i = 0; i < interrupt_routing_info.num_msi_entries; i++) {
        if (interrupt_routing_info.msi_entries[i].vector == vector && 
            interrupt_routing_info.msi_entries[i].enabled) {
            interrupt_routing_info.msi_interrupts++;
            return;
        }
    }
    
    /* Check MSI-X entries */
    for (u32 i = 0; i < interrupt_routing_info.num_msix_entries; i++) {
        if ((interrupt_routing_info.msix_entries[i].data & 0xFF) == vector && 
            !interrupt_routing_info.msix_entries[i].masked) {
            interrupt_routing_info.msix_interrupts++;
            return;
        }
    }
}

u32 interrupt_routing_get_total_interrupts_routed(void) {
    return interrupt_routing_info.total_interrupts_routed;
}

u32 interrupt_routing_get_msi_interrupts(void) {
    return interrupt_routing_info.msi_interrupts;
}

u32 interrupt_routing_get_msix_interrupts(void) {
    return interrupt_routing_info.msix_interrupts;
}

u32 interrupt_routing_get_ioapic_interrupts(void) {
    return interrupt_routing_info.ioapic_interrupts;
}

u64 interrupt_routing_get_last_interrupt_time(void) {
    return interrupt_routing_info.last_interrupt_time;
}

u32 interrupt_routing_get_num_msi_entries(void) {
    return interrupt_routing_info.num_msi_entries;
}

u32 interrupt_routing_get_num_msix_entries(void) {
    return interrupt_routing_info.num_msix_entries;
}

const msi_entry_t* interrupt_routing_get_msi_entry(u32 index) {
    if (index >= interrupt_routing_info.num_msi_entries) return 0;
    return &interrupt_routing_info.msi_entries[index];
}

const msix_entry_t* interrupt_routing_get_msix_entry(u32 index) {
    if (index >= interrupt_routing_info.num_msix_entries) return 0;
    return &interrupt_routing_info.msix_entries[index];
}

void interrupt_routing_clear_statistics(void) {
    interrupt_routing_info.total_interrupts_routed = 0;
    interrupt_routing_info.msi_interrupts = 0;
    interrupt_routing_info.msix_interrupts = 0;
    interrupt_routing_info.ioapic_interrupts = 0;
    
    for (u8 i = 0; i < interrupt_routing_info.num_ioapics; i++) {
        for (u8 j = 0; j < 24; j++) {
            interrupt_routing_info.ioapics[i].interrupt_count[j] = 0;
        }
    }
}

const char* interrupt_routing_get_delivery_mode_name(u8 delivery_mode) {
    switch (delivery_mode) {
        case DELIVERY_MODE_FIXED: return "Fixed";
        case DELIVERY_MODE_LOWEST: return "Lowest Priority";
        case DELIVERY_MODE_SMI: return "SMI";
        case DELIVERY_MODE_NMI: return "NMI";
        case DELIVERY_MODE_INIT: return "INIT";
        case DELIVERY_MODE_EXTINT: return "ExtINT";
        default: return "Unknown";
    }
}
