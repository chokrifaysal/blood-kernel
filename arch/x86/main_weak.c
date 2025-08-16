/*
 * arch/x86/main_weak.c – x86 QEMU overrides
 */

#include "kernel/types.h"

const char *arch_name(void) { return "x86-32"; }
const char *mcu_name(void)  { return "QEMU-i686"; }
const char *boot_name(void) { return "CPUFreq+IOAPIC+CPUID"; }

void vga_init(void);
void ps2_kbd_init(void);
void pci_init(void);
void ata_init(void);
void idt_init(void);
void pic_init(void);
void paging_init(u32 memory_size);
void cpuid_init(void);
void enable_interrupts(void);
void rtc_init(void);
void serial_init(u8 port, u32 baud_rate, u8 data_bits, u8 stop_bits, u8 parity);
void floppy_init(void);
void acpi_init(void);
void apic_init(u32 lapic_addr, u32 ioapic_addr);
u8 uhci_init(u16 io_base, u8 irq);
void dma_init(void);
u8 ac97_init(u16 nambar, u16 nabmbar, u8 irq);
u8 rtl8139_init(u16 io_base, u8 irq);
u8 hpet_init(u64 base_address);
void msr_init(void);
void smbios_init(void);
void thermal_init(void);
void power_init(void);
void iommu_init(void);
void cache_init(void);
void vmx_init(void);
void perfmon_init(void);
void longmode_init(void);
void microcode_init(void);
void x2apic_init(void);
void topology_init(void);
void xsave_init(void);
void numa_init(void);
void security_init(void);
void debug_init(void);
void errata_init(void);
void cpufreq_init(void);
void ioapic_init(void);
void cpuid_ext_init(void);
void x86_pc_demo_init(void);

void clock_init(void) {
    /* Initialize CPU identification */
    cpuid_init();

    /* Initialize interrupt system */
    idt_init();
    pic_init();

    /* Initialize memory management */
    paging_init(64 * 1024 * 1024); /* 64MB default */

    /* Initialize MSR support */
    msr_init();

    /* Initialize SMBIOS */
    smbios_init();

    /* Initialize thermal management */
    thermal_init();

    /* Initialize power management */
    power_init();

    /* Initialize cache management */
    cache_init();

    /* Initialize virtualization */
    vmx_init();

    /* Initialize performance monitoring */
    perfmon_init();

    /* Initialize long mode support */
    longmode_init();

    /* Initialize microcode updates */
    microcode_init();

    /* Initialize CPU topology */
    topology_init();

    /* Initialize XSAVE state management */
    xsave_init();

    /* Initialize CPU errata handling */
    errata_init();

    /* Initialize security features */
    security_init();

    /* Initialize debugging features */
    debug_init();

    /* Initialize extended CPUID features */
    cpuid_ext_init();

    /* Initialize CPU frequency scaling */
    cpufreq_init();

    /* Initialize ACPI */
    acpi_init();

    /* Initialize HPET if available */
    extern void* acpi_find_table(const char* signature);
    void* hpet_table = acpi_find_table("HPET");
    if (hpet_table) {
        u64 hpet_base = *(u64*)((u8*)hpet_table + 44); /* HPET base address */
        hpet_init(hpet_base);
    }

    /* Initialize APIC if available */
    extern u32 acpi_get_local_apic_base(void);
    extern u8 acpi_is_available(void);

    if (acpi_is_available()) {
        u32 lapic_base = acpi_get_local_apic_base();

        /* Try x2APIC first, fallback to xAPIC */
        x2apic_init();
        if (!x2apic_is_enabled()) {
            apic_init(lapic_base, 0xFEC00000); /* Standard I/O APIC address */
        }

        /* Initialize IOAPIC */
        ioapic_init();

        /* Initialize IOMMU if available */
        extern void* acpi_find_table(const char* signature);
        void* dmar_table = acpi_find_table("DMAR");
        if (dmar_table) {
            /* Parse DMAR table and initialize IOMMU units */
            extern u8 iommu_add_unit(u32 base_address, u8 segment, u8 start_bus, u8 end_bus, u8 flags);
            iommu_add_unit(0xFED90000, 0, 0, 255, 0); /* Standard IOMMU address */
            iommu_init();
        }

        /* Initialize NUMA after ACPI */
        numa_init();
    }

    /* Initialize hardware */
    rtc_init();
    serial_init(0, 115200, 8, 1, 0); /* COM1: 115200 8N1 */
    serial_init(1, 9600, 8, 1, 0);   /* COM2: 9600 8N1 */
    floppy_init();

    /* Initialize DMA controller */
    dma_init();

    /* Initialize USB controllers */
    extern pci_device_t* pci_find_class(u8 class_code, u8 subclass);
    pci_device_t* usb_dev = pci_find_class(0x0C, 0x03); /* USB controller */
    if (usb_dev && usb_dev->prog_if == 0x00) { /* UHCI */
        uhci_init(usb_dev->bar[4] & 0xFFFC, usb_dev->irq_line);
    }

    /* Initialize sound card */
    pci_device_t* audio_dev = pci_find_class(0x04, 0x01); /* Audio device */
    if (audio_dev) {
        u16 nambar = audio_dev->bar[0] & 0xFFFC;
        u16 nabmbar = audio_dev->bar[1] & 0xFFFC;
        ac97_init(nambar, nabmbar, audio_dev->irq_line);
    }

    /* Initialize network card */
    pci_device_t* net_dev = pci_find_class(0x02, 0x00); /* Ethernet controller */
    if (net_dev && net_dev->vendor_id == 0x10EC && net_dev->device_id == 0x8139) {
        u16 io_base = net_dev->bar[0] & 0xFFFC;
        rtl8139_init(io_base, net_dev->irq_line);
    }

    /* Enable interrupts */
    pic_enable_irq(0); /* Timer */
    pic_enable_irq(1); /* Keyboard */
    pic_enable_irq(3); /* COM2 */
    pic_enable_irq(4); /* COM1 */
    pic_enable_irq(5); /* Sound card */
    pic_enable_irq(6); /* Floppy */
    pic_enable_irq(8); /* RTC */
    pic_enable_irq(11); /* Network card */
    enable_interrupts();
}

void gpio_init(void) {
    /* No GPIO on PC - use PCI devices */
}

void ipc_init(void) {
    /* Single core for now */
}
