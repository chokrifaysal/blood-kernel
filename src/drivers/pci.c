/*
 * pci.c – x86 PCI bus enumeration and configuration
 */

#include "kernel/types.h"

#define PCI_CONFIG_ADDR 0xCF8
#define PCI_CONFIG_DATA 0xCFC

#define PCI_VENDOR_ID     0x00
#define PCI_DEVICE_ID     0x02
#define PCI_COMMAND       0x04
#define PCI_STATUS        0x06
#define PCI_REVISION_ID   0x08
#define PCI_PROG_IF       0x09
#define PCI_SUBCLASS      0x0A
#define PCI_CLASS_CODE    0x0B
#define PCI_CACHE_LINE    0x0C
#define PCI_LATENCY       0x0D
#define PCI_HEADER_TYPE   0x0E
#define PCI_BIST          0x0F
#define PCI_BAR0          0x10
#define PCI_BAR1          0x14
#define PCI_BAR2          0x18
#define PCI_BAR3          0x1C
#define PCI_BAR4          0x20
#define PCI_BAR5          0x24
#define PCI_INTERRUPT_LINE 0x3C
#define PCI_INTERRUPT_PIN  0x3D

typedef struct {
    u8 bus;
    u8 device;
    u8 function;
    u16 vendor_id;
    u16 device_id;
    u8 class_code;
    u8 subclass;
    u8 prog_if;
    u8 revision;
    u8 header_type;
    u32 bar[6];
    u8 irq_line;
    u8 irq_pin;
} pci_device_t;

static pci_device_t pci_devices[256];
static u16 pci_device_count = 0;

static inline void outl(u16 port, u32 val) {
    __asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline u32 inl(u16 port) {
    u32 val;
    __asm__ volatile("inl %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static u32 pci_config_read(u8 bus, u8 device, u8 function, u8 offset) {
    u32 address = (1UL << 31) | (bus << 16) | (device << 11) | 
                  (function << 8) | (offset & 0xFC);
    
    outl(PCI_CONFIG_ADDR, address);
    return inl(PCI_CONFIG_DATA) >> ((offset & 3) * 8);
}

static void pci_config_write(u8 bus, u8 device, u8 function, u8 offset, u32 value) {
    u32 address = (1UL << 31) | (bus << 16) | (device << 11) | 
                  (function << 8) | (offset & 0xFC);
    
    outl(PCI_CONFIG_ADDR, address);
    outl(PCI_CONFIG_DATA, value);
}

static u16 pci_config_read16(u8 bus, u8 device, u8 function, u8 offset) {
    return pci_config_read(bus, device, function, offset) & 0xFFFF;
}

static u8 pci_config_read8(u8 bus, u8 device, u8 function, u8 offset) {
    return pci_config_read(bus, device, function, offset) & 0xFF;
}

static void pci_config_write16(u8 bus, u8 device, u8 function, u8 offset, u16 value) {
    u32 tmp = pci_config_read(bus, device, function, offset & 0xFC);
    tmp &= ~(0xFFFF << ((offset & 2) * 8));
    tmp |= value << ((offset & 2) * 8);
    pci_config_write(bus, device, function, offset & 0xFC, tmp);
}

static u32 pci_get_bar_size(u8 bus, u8 device, u8 function, u8 bar_num) {
    u8 bar_offset = PCI_BAR0 + (bar_num * 4);
    
    /* Save original value */
    u32 original = pci_config_read(bus, device, function, bar_offset);
    
    /* Write all 1s */
    pci_config_write(bus, device, function, bar_offset, 0xFFFFFFFF);
    
    /* Read back to get size */
    u32 size = pci_config_read(bus, device, function, bar_offset);
    
    /* Restore original value */
    pci_config_write(bus, device, function, bar_offset, original);
    
    if (size == 0) return 0;
    
    /* Calculate size */
    if (original & 1) {
        /* I/O space */
        size &= 0xFFFFFFFC;
        return (~size) + 1;
    } else {
        /* Memory space */
        size &= 0xFFFFFFF0;
        return (~size) + 1;
    }
}

static void pci_scan_device(u8 bus, u8 device) {
    u16 vendor_id = pci_config_read16(bus, device, 0, PCI_VENDOR_ID);
    
    if (vendor_id == 0xFFFF) return; /* No device */
    
    pci_device_t* dev = &pci_devices[pci_device_count++];
    
    dev->bus = bus;
    dev->device = device;
    dev->function = 0;
    dev->vendor_id = vendor_id;
    dev->device_id = pci_config_read16(bus, device, 0, PCI_DEVICE_ID);
    dev->class_code = pci_config_read8(bus, device, 0, PCI_CLASS_CODE);
    dev->subclass = pci_config_read8(bus, device, 0, PCI_SUBCLASS);
    dev->prog_if = pci_config_read8(bus, device, 0, PCI_PROG_IF);
    dev->revision = pci_config_read8(bus, device, 0, PCI_REVISION_ID);
    dev->header_type = pci_config_read8(bus, device, 0, PCI_HEADER_TYPE);
    dev->irq_line = pci_config_read8(bus, device, 0, PCI_INTERRUPT_LINE);
    dev->irq_pin = pci_config_read8(bus, device, 0, PCI_INTERRUPT_PIN);
    
    /* Read BARs */
    for (u8 i = 0; i < 6; i++) {
        dev->bar[i] = pci_config_read(bus, device, 0, PCI_BAR0 + (i * 4));
    }
    
    /* Check for multi-function device */
    if (dev->header_type & 0x80) {
        for (u8 func = 1; func < 8; func++) {
            u16 func_vendor = pci_config_read16(bus, device, func, PCI_VENDOR_ID);
            if (func_vendor != 0xFFFF && pci_device_count < 256) {
                pci_device_t* func_dev = &pci_devices[pci_device_count++];
                
                func_dev->bus = bus;
                func_dev->device = device;
                func_dev->function = func;
                func_dev->vendor_id = func_vendor;
                func_dev->device_id = pci_config_read16(bus, device, func, PCI_DEVICE_ID);
                func_dev->class_code = pci_config_read8(bus, device, func, PCI_CLASS_CODE);
                func_dev->subclass = pci_config_read8(bus, device, func, PCI_SUBCLASS);
                func_dev->prog_if = pci_config_read8(bus, device, func, PCI_PROG_IF);
                func_dev->revision = pci_config_read8(bus, device, func, PCI_REVISION_ID);
                func_dev->header_type = pci_config_read8(bus, device, func, PCI_HEADER_TYPE);
                func_dev->irq_line = pci_config_read8(bus, device, func, PCI_INTERRUPT_LINE);
                func_dev->irq_pin = pci_config_read8(bus, device, func, PCI_INTERRUPT_PIN);
                
                for (u8 i = 0; i < 6; i++) {
                    func_dev->bar[i] = pci_config_read(bus, device, func, PCI_BAR0 + (i * 4));
                }
            }
        }
    }
}

void pci_init(void) {
    pci_device_count = 0;
    
    /* Scan all buses and devices */
    for (u16 bus = 0; bus < 256; bus++) {
        for (u8 device = 0; device < 32; device++) {
            if (pci_device_count >= 256) break;
            pci_scan_device(bus, device);
        }
        if (pci_device_count >= 256) break;
    }
}

u16 pci_get_device_count(void) {
    return pci_device_count;
}

pci_device_t* pci_get_device(u16 index) {
    if (index < pci_device_count) {
        return &pci_devices[index];
    }
    return 0;
}

pci_device_t* pci_find_device(u16 vendor_id, u16 device_id) {
    for (u16 i = 0; i < pci_device_count; i++) {
        if (pci_devices[i].vendor_id == vendor_id && 
            pci_devices[i].device_id == device_id) {
            return &pci_devices[i];
        }
    }
    return 0;
}

pci_device_t* pci_find_class(u8 class_code, u8 subclass) {
    for (u16 i = 0; i < pci_device_count; i++) {
        if (pci_devices[i].class_code == class_code && 
            pci_devices[i].subclass == subclass) {
            return &pci_devices[i];
        }
    }
    return 0;
}

void pci_enable_device(pci_device_t* dev) {
    u16 command = pci_config_read16(dev->bus, dev->device, dev->function, PCI_COMMAND);
    command |= 0x07; /* Enable I/O space, memory space, and bus mastering */
    pci_config_write16(dev->bus, dev->device, dev->function, PCI_COMMAND, command);
}

void pci_set_irq_line(pci_device_t* dev, u8 irq) {
    pci_config_write(dev->bus, dev->device, dev->function, PCI_INTERRUPT_LINE, irq);
    dev->irq_line = irq;
}

const char* pci_get_class_name(u8 class_code) {
    switch (class_code) {
        case 0x00: return "Legacy";
        case 0x01: return "Storage";
        case 0x02: return "Network";
        case 0x03: return "Display";
        case 0x04: return "Multimedia";
        case 0x05: return "Memory";
        case 0x06: return "Bridge";
        case 0x07: return "Communication";
        case 0x08: return "System";
        case 0x09: return "Input";
        case 0x0A: return "Docking";
        case 0x0B: return "Processor";
        case 0x0C: return "Serial Bus";
        case 0x0D: return "Wireless";
        case 0x0E: return "Intelligent";
        case 0x0F: return "Satellite";
        case 0x10: return "Encryption";
        case 0x11: return "Signal Processing";
        default: return "Unknown";
    }
}
