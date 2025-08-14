/*
 * acpi.c – x86 Advanced Configuration and Power Interface
 */

#include "kernel/types.h"

#define ACPI_RSDP_SIGNATURE "RSD PTR "
#define ACPI_RSDT_SIGNATURE "RSDT"
#define ACPI_XSDT_SIGNATURE "XSDT"
#define ACPI_FADT_SIGNATURE "FACP"
#define ACPI_MADT_SIGNATURE "APIC"
#define ACPI_HPET_SIGNATURE "HPET"
#define ACPI_MCFG_SIGNATURE "MCFG"

/* ACPI table headers */
typedef struct {
    char signature[4];
    u32 length;
    u8 revision;
    u8 checksum;
    char oem_id[6];
    char oem_table_id[8];
    u32 oem_revision;
    u32 creator_id;
    u32 creator_revision;
} __attribute__((packed)) acpi_header_t;

/* Root System Description Pointer */
typedef struct {
    char signature[8];
    u8 checksum;
    char oem_id[6];
    u8 revision;
    u32 rsdt_address;
    u32 length;
    u64 xsdt_address;
    u8 extended_checksum;
    u8 reserved[3];
} __attribute__((packed)) acpi_rsdp_t;

/* Root System Description Table */
typedef struct {
    acpi_header_t header;
    u32 tables[];
} __attribute__((packed)) acpi_rsdt_t;

/* Fixed ACPI Description Table */
typedef struct {
    acpi_header_t header;
    u32 firmware_ctrl;
    u32 dsdt;
    u8 reserved1;
    u8 preferred_pm_profile;
    u16 sci_interrupt;
    u32 smi_command_port;
    u8 acpi_enable;
    u8 acpi_disable;
    u8 s4bios_req;
    u8 pstate_control;
    u32 pm1a_event_block;
    u32 pm1b_event_block;
    u32 pm1a_control_block;
    u32 pm1b_control_block;
    u32 pm2_control_block;
    u32 pm_timer_block;
    u32 gpe0_block;
    u32 gpe1_block;
    u8 pm1_event_length;
    u8 pm1_control_length;
    u8 pm2_control_length;
    u8 pm_timer_length;
    u8 gpe0_length;
    u8 gpe1_length;
    u8 gpe1_base;
    u8 cstate_control;
    u16 worst_c2_latency;
    u16 worst_c3_latency;
    u16 flush_size;
    u16 flush_stride;
    u8 duty_offset;
    u8 duty_width;
    u8 day_alarm;
    u8 month_alarm;
    u8 century;
    u16 boot_architecture_flags;
    u8 reserved2;
    u32 flags;
} __attribute__((packed)) acpi_fadt_t;

/* Multiple APIC Description Table */
typedef struct {
    acpi_header_t header;
    u32 local_apic_address;
    u32 flags;
    u8 entries[];
} __attribute__((packed)) acpi_madt_t;

/* MADT entry types */
#define MADT_TYPE_LOCAL_APIC     0
#define MADT_TYPE_IO_APIC        1
#define MADT_TYPE_INT_OVERRIDE   2
#define MADT_TYPE_NMI_SOURCE     3
#define MADT_TYPE_LOCAL_NMI      4

typedef struct {
    u8 type;
    u8 length;
    u8 processor_id;
    u8 apic_id;
    u32 flags;
} __attribute__((packed)) madt_local_apic_t;

typedef struct {
    u8 type;
    u8 length;
    u8 io_apic_id;
    u8 reserved;
    u32 io_apic_address;
    u32 global_system_interrupt_base;
} __attribute__((packed)) madt_io_apic_t;

typedef struct {
    u8 type;
    u8 length;
    u8 bus;
    u8 source;
    u32 global_system_interrupt;
    u16 flags;
} __attribute__((packed)) madt_int_override_t;

static acpi_rsdp_t* acpi_rsdp = 0;
static acpi_rsdt_t* acpi_rsdt = 0;
static acpi_fadt_t* acpi_fadt = 0;
static acpi_madt_t* acpi_madt = 0;
static u32 local_apic_base = 0;
static u8 cpu_count = 0;

static u8 acpi_checksum(void* ptr, u32 length) {
    u8* data = (u8*)ptr;
    u8 sum = 0;
    for (u32 i = 0; i < length; i++) {
        sum += data[i];
    }
    return sum;
}

static acpi_rsdp_t* acpi_find_rsdp(void) {
    /* Search EBDA */
    u16 ebda = *(u16*)0x40E;
    if (ebda) {
        ebda <<= 4;
        for (u32 addr = ebda; addr < ebda + 1024; addr += 16) {
            if (*(u64*)addr == *(u64*)ACPI_RSDP_SIGNATURE) {
                acpi_rsdp_t* rsdp = (acpi_rsdp_t*)addr;
                if (acpi_checksum(rsdp, 20) == 0) {
                    return rsdp;
                }
            }
        }
    }
    
    /* Search BIOS area */
    for (u32 addr = 0xE0000; addr < 0x100000; addr += 16) {
        if (*(u64*)addr == *(u64*)ACPI_RSDP_SIGNATURE) {
            acpi_rsdp_t* rsdp = (acpi_rsdp_t*)addr;
            if (acpi_checksum(rsdp, 20) == 0) {
                return rsdp;
            }
        }
    }
    
    return 0;
}

static void* acpi_find_table(const char* signature) {
    if (!acpi_rsdt) return 0;
    
    u32 entries = (acpi_rsdt->header.length - sizeof(acpi_header_t)) / 4;
    
    for (u32 i = 0; i < entries; i++) {
        acpi_header_t* header = (acpi_header_t*)acpi_rsdt->tables[i];
        if (*(u32*)header->signature == *(u32*)signature) {
            if (acpi_checksum(header, header->length) == 0) {
                return header;
            }
        }
    }
    
    return 0;
}

void acpi_init(void) {
    /* Find RSDP */
    acpi_rsdp = acpi_find_rsdp();
    if (!acpi_rsdp) {
        return;
    }
    
    /* Get RSDT */
    acpi_rsdt = (acpi_rsdt_t*)acpi_rsdp->rsdt_address;
    if (!acpi_rsdt || acpi_checksum(acpi_rsdt, acpi_rsdt->header.length) != 0) {
        return;
    }
    
    /* Find FADT */
    acpi_fadt = (acpi_fadt_t*)acpi_find_table(ACPI_FADT_SIGNATURE);
    
    /* Find MADT */
    acpi_madt = (acpi_madt_t*)acpi_find_table(ACPI_MADT_SIGNATURE);
    if (acpi_madt) {
        local_apic_base = acpi_madt->local_apic_address;
        
        /* Count CPUs */
        u8* entry = acpi_madt->entries;
        u8* end = (u8*)acpi_madt + acpi_madt->header.length;
        
        while (entry < end) {
            if (entry[0] == MADT_TYPE_LOCAL_APIC) {
                madt_local_apic_t* lapic = (madt_local_apic_t*)entry;
                if (lapic->flags & 1) {
                    cpu_count++;
                }
            }
            entry += entry[1];
        }
    }
}

u8 acpi_is_available(void) {
    return acpi_rsdp != 0;
}

u32 acpi_get_local_apic_base(void) {
    return local_apic_base;
}

u8 acpi_get_cpu_count(void) {
    return cpu_count;
}

void acpi_enable(void) {
    if (!acpi_fadt || !acpi_fadt->smi_command_port) {
        return;
    }
    
    /* Enable ACPI mode */
    __asm__ volatile("outb %0, %1" : : "a"(acpi_fadt->acpi_enable), "Nd"(acpi_fadt->smi_command_port));
    
    /* Wait for ACPI to be enabled */
    u32 timeout = 1000000;
    while (timeout-- && !acpi_is_enabled());
}

u8 acpi_is_enabled(void) {
    if (!acpi_fadt) return 0;
    
    u16 pm1_control = 0;
    if (acpi_fadt->pm1a_control_block) {
        __asm__ volatile("inw %1, %0" : "=a"(pm1_control) : "Nd"(acpi_fadt->pm1a_control_block));
    }
    
    return (pm1_control & 1) != 0;
}

void acpi_power_off(void) {
    if (!acpi_fadt || !acpi_is_enabled()) {
        return;
    }
    
    /* Set SLP_TYP to S5 and SLP_EN */
    u16 pm1_control = 0x2000 | 0x2000; /* S5 state */
    
    if (acpi_fadt->pm1a_control_block) {
        __asm__ volatile("outw %0, %1" : : "a"(pm1_control), "Nd"(acpi_fadt->pm1a_control_block));
    }
    
    if (acpi_fadt->pm1b_control_block) {
        __asm__ volatile("outw %0, %1" : : "a"(pm1_control), "Nd"(acpi_fadt->pm1b_control_block));
    }
    
    /* Should not reach here */
    while (1) {
        __asm__ volatile("hlt");
    }
}

void acpi_reboot(void) {
    if (!acpi_fadt) {
        /* Fallback to keyboard controller reset */
        __asm__ volatile("outb $0xFE, $0x64");
        return;
    }
    
    /* Try ACPI reset register */
    if (acpi_fadt->flags & (1 << 10)) {
        /* Reset register supported */
        __asm__ volatile("outb $0x06, $0xCF9"); /* Full reset */
    } else {
        /* Fallback methods */
        __asm__ volatile("outb $0xFE, $0x64"); /* Keyboard controller */
    }
    
    while (1) {
        __asm__ volatile("hlt");
    }
}

u32 acpi_get_pm_timer(void) {
    if (!acpi_fadt || !acpi_fadt->pm_timer_block) {
        return 0;
    }
    
    u32 timer_value;
    __asm__ volatile("inl %1, %0" : "=a"(timer_value) : "Nd"(acpi_fadt->pm_timer_block));
    
    return timer_value;
}

void acpi_enumerate_io_apics(void (*callback)(u8 id, u32 address, u32 gsi_base)) {
    if (!acpi_madt || !callback) return;
    
    u8* entry = acpi_madt->entries;
    u8* end = (u8*)acpi_madt + acpi_madt->header.length;
    
    while (entry < end) {
        if (entry[0] == MADT_TYPE_IO_APIC) {
            madt_io_apic_t* ioapic = (madt_io_apic_t*)entry;
            callback(ioapic->io_apic_id, ioapic->io_apic_address, 
                    ioapic->global_system_interrupt_base);
        }
        entry += entry[1];
    }
}

void acpi_enumerate_interrupt_overrides(void (*callback)(u8 source, u32 gsi, u16 flags)) {
    if (!acpi_madt || !callback) return;
    
    u8* entry = acpi_madt->entries;
    u8* end = (u8*)acpi_madt + acpi_madt->header.length;
    
    while (entry < end) {
        if (entry[0] == MADT_TYPE_INT_OVERRIDE) {
            madt_int_override_t* override = (madt_int_override_t*)entry;
            callback(override->source, override->global_system_interrupt, 
                    override->flags);
        }
        entry += entry[1];
    }
}
