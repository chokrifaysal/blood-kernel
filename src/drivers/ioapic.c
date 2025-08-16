/*
 * ioapic.c – x86 I/O Advanced Programmable Interrupt Controller
 */

#include "kernel/types.h"

/* IOAPIC register offsets */
#define IOAPIC_REGSEL           0x00
#define IOAPIC_REGWIN           0x10

/* IOAPIC registers */
#define IOAPIC_ID               0x00
#define IOAPIC_VER              0x01
#define IOAPIC_ARB              0x02
#define IOAPIC_REDTBL_BASE      0x10

/* Redirection table entry bits */
#define IOAPIC_DEST_PHYSICAL    0x00000000
#define IOAPIC_DEST_LOGICAL     0x00000800
#define IOAPIC_INTPOL_HIGH      0x00000000
#define IOAPIC_INTPOL_LOW       0x00002000
#define IOAPIC_TRIGGER_EDGE     0x00000000
#define IOAPIC_TRIGGER_LEVEL    0x00008000
#define IOAPIC_MASK             0x00010000
#define IOAPIC_DELIVERY_FIXED   0x00000000
#define IOAPIC_DELIVERY_LOWEST  0x00000100
#define IOAPIC_DELIVERY_SMI     0x00000200
#define IOAPIC_DELIVERY_NMI     0x00000400
#define IOAPIC_DELIVERY_INIT    0x00000500
#define IOAPIC_DELIVERY_EXTINT  0x00000700

typedef struct {
    u32 low;
    u32 high;
} ioapic_redir_entry_t;

typedef struct {
    u32 base_address;
    u8 id;
    u8 version;
    u8 max_redirection_entries;
    u8 enabled;
    u32 gsi_base;
    ioapic_redir_entry_t redirection_table[24];
} ioapic_controller_t;

typedef struct {
    u8 num_ioapics;
    ioapic_controller_t ioapics[8];
    u8 initialized;
} ioapic_info_t;

static ioapic_info_t ioapic_info;

static u32 ioapic_read_reg(u32 base, u8 reg) {
    volatile u32* regsel = (volatile u32*)base;
    volatile u32* regwin = (volatile u32*)(base + IOAPIC_REGWIN);
    
    *regsel = reg;
    return *regwin;
}

static void ioapic_write_reg(u32 base, u8 reg, u32 value) {
    volatile u32* regsel = (volatile u32*)base;
    volatile u32* regwin = (volatile u32*)(base + IOAPIC_REGWIN);
    
    *regsel = reg;
    *regwin = value;
}

static void ioapic_read_redirection_entry(u32 base, u8 irq, ioapic_redir_entry_t* entry) {
    entry->low = ioapic_read_reg(base, IOAPIC_REDTBL_BASE + irq * 2);
    entry->high = ioapic_read_reg(base, IOAPIC_REDTBL_BASE + irq * 2 + 1);
}

static void ioapic_write_redirection_entry(u32 base, u8 irq, const ioapic_redir_entry_t* entry) {
    ioapic_write_reg(base, IOAPIC_REDTBL_BASE + irq * 2, entry->low);
    ioapic_write_reg(base, IOAPIC_REDTBL_BASE + irq * 2 + 1, entry->high);
}

static void ioapic_detect_controllers(void) {
    extern void* acpi_find_table(const char* signature);
    void* madt_table = acpi_find_table("APIC");
    
    if (!madt_table) {
        /* Fallback to standard IOAPIC address */
        ioapic_controller_t* ioapic = &ioapic_info.ioapics[0];
        ioapic->base_address = 0xFEC00000;
        ioapic->gsi_base = 0;
        ioapic->enabled = 1;
        ioapic_info.num_ioapics = 1;
        return;
    }
    
    u8* madt_data = (u8*)madt_table;
    u32 madt_length = *(u32*)(madt_data + 4);
    u32 offset = 44; /* Skip MADT header */
    
    ioapic_info.num_ioapics = 0;
    
    while (offset < madt_length && ioapic_info.num_ioapics < 8) {
        u8 entry_type = madt_data[offset];
        u8 entry_length = madt_data[offset + 1];
        
        if (entry_type == 1) { /* IOAPIC entry */
            ioapic_controller_t* ioapic = &ioapic_info.ioapics[ioapic_info.num_ioapics];
            
            ioapic->id = madt_data[offset + 2];
            ioapic->base_address = *(u32*)(madt_data + offset + 4);
            ioapic->gsi_base = *(u32*)(madt_data + offset + 8);
            ioapic->enabled = 1;
            
            ioapic_info.num_ioapics++;
        }
        
        offset += entry_length;
    }
    
    if (ioapic_info.num_ioapics == 0) {
        /* No IOAPIC found in MADT, use default */
        ioapic_controller_t* ioapic = &ioapic_info.ioapics[0];
        ioapic->base_address = 0xFEC00000;
        ioapic->gsi_base = 0;
        ioapic->enabled = 1;
        ioapic_info.num_ioapics = 1;
    }
}

static void ioapic_initialize_controller(ioapic_controller_t* ioapic) {
    if (!ioapic->enabled) return;
    
    /* Read IOAPIC version and capabilities */
    u32 version_reg = ioapic_read_reg(ioapic->base_address, IOAPIC_VER);
    ioapic->version = version_reg & 0xFF;
    ioapic->max_redirection_entries = ((version_reg >> 16) & 0xFF) + 1;
    
    /* Mask all interrupts initially */
    for (u8 i = 0; i < ioapic->max_redirection_entries; i++) {
        ioapic_redir_entry_t entry;
        entry.low = IOAPIC_MASK | IOAPIC_DELIVERY_FIXED | (0x20 + i); /* Vector 0x20+ */
        entry.high = 0; /* Destination APIC ID 0 */
        
        ioapic_write_redirection_entry(ioapic->base_address, i, &entry);
        ioapic->redirection_table[i] = entry;
    }
}

void ioapic_init(void) {
    ioapic_info.num_ioapics = 0;
    ioapic_info.initialized = 0;
    
    ioapic_detect_controllers();
    
    for (u8 i = 0; i < ioapic_info.num_ioapics; i++) {
        ioapic_initialize_controller(&ioapic_info.ioapics[i]);
    }
    
    ioapic_info.initialized = 1;
}

u8 ioapic_is_initialized(void) {
    return ioapic_info.initialized;
}

u8 ioapic_get_num_controllers(void) {
    return ioapic_info.num_ioapics;
}

u32 ioapic_get_controller_base(u8 controller_id) {
    if (controller_id >= ioapic_info.num_ioapics) return 0;
    return ioapic_info.ioapics[controller_id].base_address;
}

u8 ioapic_get_controller_id(u8 controller_id) {
    if (controller_id >= ioapic_info.num_ioapics) return 0;
    return ioapic_info.ioapics[controller_id].id;
}

u8 ioapic_get_max_redirection_entries(u8 controller_id) {
    if (controller_id >= ioapic_info.num_ioapics) return 0;
    return ioapic_info.ioapics[controller_id].max_redirection_entries;
}

u32 ioapic_get_gsi_base(u8 controller_id) {
    if (controller_id >= ioapic_info.num_ioapics) return 0;
    return ioapic_info.ioapics[controller_id].gsi_base;
}

static u8 ioapic_find_controller_for_gsi(u32 gsi, u8* irq_pin) {
    for (u8 i = 0; i < ioapic_info.num_ioapics; i++) {
        ioapic_controller_t* ioapic = &ioapic_info.ioapics[i];
        u32 gsi_end = ioapic->gsi_base + ioapic->max_redirection_entries - 1;
        
        if (gsi >= ioapic->gsi_base && gsi <= gsi_end) {
            *irq_pin = gsi - ioapic->gsi_base;
            return i;
        }
    }
    
    return 0xFF; /* Not found */
}

void ioapic_set_irq(u32 gsi, u8 vector, u8 dest_apic_id, u8 trigger_mode, u8 polarity) {
    if (!ioapic_info.initialized) return;
    
    u8 irq_pin;
    u8 controller_id = ioapic_find_controller_for_gsi(gsi, &irq_pin);
    
    if (controller_id == 0xFF) return;
    
    ioapic_controller_t* ioapic = &ioapic_info.ioapics[controller_id];
    
    ioapic_redir_entry_t entry;
    entry.low = vector | IOAPIC_DELIVERY_FIXED | IOAPIC_DEST_PHYSICAL;
    entry.high = (u32)dest_apic_id << 24;
    
    if (trigger_mode) {
        entry.low |= IOAPIC_TRIGGER_LEVEL;
    } else {
        entry.low |= IOAPIC_TRIGGER_EDGE;
    }
    
    if (polarity) {
        entry.low |= IOAPIC_INTPOL_LOW;
    } else {
        entry.low |= IOAPIC_INTPOL_HIGH;
    }
    
    ioapic_write_redirection_entry(ioapic->base_address, irq_pin, &entry);
    ioapic->redirection_table[irq_pin] = entry;
}

void ioapic_mask_irq(u32 gsi) {
    if (!ioapic_info.initialized) return;
    
    u8 irq_pin;
    u8 controller_id = ioapic_find_controller_for_gsi(gsi, &irq_pin);
    
    if (controller_id == 0xFF) return;
    
    ioapic_controller_t* ioapic = &ioapic_info.ioapics[controller_id];
    
    ioapic_redir_entry_t entry;
    ioapic_read_redirection_entry(ioapic->base_address, irq_pin, &entry);
    
    entry.low |= IOAPIC_MASK;
    
    ioapic_write_redirection_entry(ioapic->base_address, irq_pin, &entry);
    ioapic->redirection_table[irq_pin] = entry;
}

void ioapic_unmask_irq(u32 gsi) {
    if (!ioapic_info.initialized) return;
    
    u8 irq_pin;
    u8 controller_id = ioapic_find_controller_for_gsi(gsi, &irq_pin);
    
    if (controller_id == 0xFF) return;
    
    ioapic_controller_t* ioapic = &ioapic_info.ioapics[controller_id];
    
    ioapic_redir_entry_t entry;
    ioapic_read_redirection_entry(ioapic->base_address, irq_pin, &entry);
    
    entry.low &= ~IOAPIC_MASK;
    
    ioapic_write_redirection_entry(ioapic->base_address, irq_pin, &entry);
    ioapic->redirection_table[irq_pin] = entry;
}

u8 ioapic_is_irq_masked(u32 gsi) {
    if (!ioapic_info.initialized) return 1;
    
    u8 irq_pin;
    u8 controller_id = ioapic_find_controller_for_gsi(gsi, &irq_pin);
    
    if (controller_id == 0xFF) return 1;
    
    ioapic_controller_t* ioapic = &ioapic_info.ioapics[controller_id];
    
    ioapic_redir_entry_t entry;
    ioapic_read_redirection_entry(ioapic->base_address, irq_pin, &entry);
    
    return (entry.low & IOAPIC_MASK) != 0;
}

void ioapic_set_delivery_mode(u32 gsi, u8 delivery_mode) {
    if (!ioapic_info.initialized) return;
    
    u8 irq_pin;
    u8 controller_id = ioapic_find_controller_for_gsi(gsi, &irq_pin);
    
    if (controller_id == 0xFF) return;
    
    ioapic_controller_t* ioapic = &ioapic_info.ioapics[controller_id];
    
    ioapic_redir_entry_t entry;
    ioapic_read_redirection_entry(ioapic->base_address, irq_pin, &entry);
    
    entry.low = (entry.low & ~0x700) | (delivery_mode << 8);
    
    ioapic_write_redirection_entry(ioapic->base_address, irq_pin, &entry);
    ioapic->redirection_table[irq_pin] = entry;
}

void ioapic_set_destination_mode(u32 gsi, u8 logical_mode) {
    if (!ioapic_info.initialized) return;
    
    u8 irq_pin;
    u8 controller_id = ioapic_find_controller_for_gsi(gsi, &irq_pin);
    
    if (controller_id == 0xFF) return;
    
    ioapic_controller_t* ioapic = &ioapic_info.ioapics[controller_id];
    
    ioapic_redir_entry_t entry;
    ioapic_read_redirection_entry(ioapic->base_address, irq_pin, &entry);
    
    if (logical_mode) {
        entry.low |= IOAPIC_DEST_LOGICAL;
    } else {
        entry.low &= ~IOAPIC_DEST_LOGICAL;
    }
    
    ioapic_write_redirection_entry(ioapic->base_address, irq_pin, &entry);
    ioapic->redirection_table[irq_pin] = entry;
}

u8 ioapic_get_vector(u32 gsi) {
    if (!ioapic_info.initialized) return 0;
    
    u8 irq_pin;
    u8 controller_id = ioapic_find_controller_for_gsi(gsi, &irq_pin);
    
    if (controller_id == 0xFF) return 0;
    
    ioapic_controller_t* ioapic = &ioapic_info.ioapics[controller_id];
    
    return ioapic->redirection_table[irq_pin].low & 0xFF;
}

u8 ioapic_get_destination(u32 gsi) {
    if (!ioapic_info.initialized) return 0;
    
    u8 irq_pin;
    u8 controller_id = ioapic_find_controller_for_gsi(gsi, &irq_pin);
    
    if (controller_id == 0xFF) return 0;
    
    ioapic_controller_t* ioapic = &ioapic_info.ioapics[controller_id];
    
    return (ioapic->redirection_table[irq_pin].high >> 24) & 0xFF;
}

void ioapic_disable_legacy_pic(void) {
    /* Mask all legacy PIC interrupts */
    extern void pic_mask_irq(u8 irq);
    
    for (u8 i = 0; i < 16; i++) {
        pic_mask_irq(i);
    }
}

void ioapic_setup_legacy_irqs(void) {
    if (!ioapic_info.initialized) return;
    
    /* Setup standard ISA IRQs */
    ioapic_set_irq(0, 0x20, 0, 0, 0);  /* Timer */
    ioapic_set_irq(1, 0x21, 0, 0, 0);  /* Keyboard */
    ioapic_set_irq(3, 0x23, 0, 0, 0);  /* COM2/COM4 */
    ioapic_set_irq(4, 0x24, 0, 0, 0);  /* COM1/COM3 */
    ioapic_set_irq(6, 0x26, 0, 0, 0);  /* Floppy */
    ioapic_set_irq(8, 0x28, 0, 0, 0);  /* RTC */
    ioapic_set_irq(12, 0x2C, 0, 0, 0); /* PS/2 Mouse */
    ioapic_set_irq(13, 0x2D, 0, 0, 0); /* FPU */
    ioapic_set_irq(14, 0x2E, 0, 0, 0); /* Primary ATA */
    ioapic_set_irq(15, 0x2F, 0, 0, 0); /* Secondary ATA */
}
