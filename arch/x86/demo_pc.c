/*
 * demo_pc.c – x86 PC hardware demo
 */

#include "kernel/types.h"
#include "drivers/vga.h"
#include "drivers/ps2_kbd.h"
#include "drivers/pci.h"
#include "drivers/pit.h"
#include "drivers/ata.h"
#include "drivers/cpuid.h"
#include "drivers/paging.h"
#include "drivers/pic.h"
#include "drivers/rtc.h"
#include "drivers/serial.h"
#include "drivers/floppy.h"
#include "drivers/acpi.h"
#include "drivers/apic.h"
#include "drivers/usb_uhci.h"
#include "drivers/dma.h"
#include "drivers/ac97.h"
#include "drivers/rtl8139.h"
#include "drivers/hpet.h"
#include "drivers/msr.h"
#include "drivers/smbios.h"
#include "drivers/thermal.h"
#include "drivers/power.h"
#include "drivers/iommu.h"
#include "drivers/cache.h"
#include "drivers/vmx.h"
#include "drivers/perfmon.h"
#include "drivers/longmode.h"
#include "drivers/microcode.h"
#include "drivers/x2apic.h"
#include "drivers/topology.h"
#include "drivers/xsave.h"
#include "drivers/numa.h"

extern void timer_delay(u32 ms);

static void vga_demo_task(void) {
    vga_init();
    vga_enable_cursor();
    
    /* Draw title box */
    vga_set_color(VGA_YELLOW, VGA_BLUE);
    vga_draw_box(0, 0, 80, 3, VGA_YELLOW | (VGA_BLUE << 4));
    vga_puts_at("Blood Kernel x86 PC Demo", 1, 28, VGA_YELLOW | (VGA_BLUE << 4));
    
    /* System info */
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_set_cursor(4, 0);
    vga_puts("System Information:\n");
    vga_puts("- VGA Text Mode: 80x25\n");
    vga_puts("- IDT: 256 entries\n");
    vga_puts("- PIC: 8259A IRQ controller\n");
    vga_puts("- MMU: 4KB paging enabled\n");
    vga_puts("- Topology: CPU/NUMA detection\n");
    vga_puts("- XSAVE: Extended state mgmt\n");
    vga_puts("- NUMA: Memory affinity control\n");
    vga_puts("- LongMode: x86-64 64-bit support\n");
    vga_puts("- Microcode: CPU firmware updates\n");
    vga_puts("- x2APIC: Advanced interrupt ctrl\n");
    vga_puts("- Cache: MTRR/PAT memory types\n");
    vga_puts("- VMX: Intel VT-x virtualization\n");
    vga_puts("- PerfMon: CPU performance counters\n");
    vga_puts("- Thermal: CPU temperature monitor\n");
    vga_puts("- Power: P-states/C-states mgmt\n");
    vga_puts("- IOMMU: VT-d/AMD-Vi translation\n");
    vga_puts("- HPET: High Precision Timer\n");
    vga_puts("- MSR: Model Specific Registers\n");
    vga_puts("- SMBIOS: System Management BIOS\n");
    vga_puts("- DMA: 8237A controller\n");
    vga_puts("- AC97: Audio codec\n");
    vga_puts("- RTL8139: Fast Ethernet\n");
    vga_puts("- ACPI: Advanced Config & Power\n");
    vga_puts("- APIC: Advanced Interrupt Ctrl\n");
    vga_puts("- USB: UHCI host controller\n");
    vga_puts("- RTC: MC146818 real-time clock\n");
    vga_puts("- COM1/COM2: 16550 UART\n");
    vga_puts("- FDC: 82077AA floppy controller\n");
    vga_puts("- PIT Timer: 1 kHz\n");
    vga_puts("- PS/2 Keyboard: Enabled\n");
    vga_puts("- PCI Bus: Scanning...\n");
    vga_puts("- ATA/IDE: Detecting...\n\n");
    
    /* Color test */
    vga_puts("Color Test:\n");
    for (u8 i = 0; i < 16; i++) {
        vga_set_color(i, VGA_BLACK);
        vga_printf("Color %d ", i);
    }
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_puts("\n\n");
    
    /* Status bar */
    vga_fill_rect(24, 0, 80, 1, ' ', VGA_BLACK | (VGA_LGRAY << 4));
    vga_puts_at("Press any key to test keyboard...", 24, 2, VGA_BLACK | (VGA_LGRAY << 4));
    
    while (1) {
        timer_delay(1000);
    }
}

static void keyboard_demo_task(void) {
    ps2_kbd_init();
    
    vga_set_cursor(15, 0);
    vga_puts("Keyboard Test (ESC to exit):\n");
    
    while (1) {
        if (ps2_kbd_available()) {
            char c = ps2_kbd_getc();
            
            if (c == 27) { /* ESC */
                vga_puts("\nKeyboard test ended.\n");
                break;
            }
            
            if (c >= 32 && c <= 126) {
                vga_putc(c);
            } else if (c == '\n') {
                vga_putc('\n');
            } else if (c == '\b') {
                vga_putc('\b');
            }
            
            /* Show modifiers */
            u8 mods = ps2_kbd_get_modifiers();
            char mod_str[20] = "";
            if (mods & KBD_MOD_SHIFT) strcat(mod_str, "SHIFT ");
            if (mods & KBD_MOD_CTRL)  strcat(mod_str, "CTRL ");
            if (mods & KBD_MOD_ALT)   strcat(mod_str, "ALT ");
            if (mods & KBD_MOD_CAPS)  strcat(mod_str, "CAPS ");
            
            vga_puts_at(mod_str, 23, 60, VGA_CYAN | (VGA_BLACK << 4));
        }
        
        timer_delay(10);
    }
}

static void pci_demo_task(void) {
    pci_init();
    
    u16 device_count = pci_get_device_count();
    
    vga_set_cursor(8, 40);
    vga_printf("PCI Devices Found: %d\n", device_count);
    
    for (u16 i = 0; i < device_count && i < 10; i++) {
        pci_device_t* dev = pci_get_device(i);
        if (dev) {
            vga_printf("%02X:%02X.%X %04X:%04X %s\n",
                      dev->bus, dev->device, dev->function,
                      dev->vendor_id, dev->device_id,
                      pci_get_class_name(dev->class_code));
        }
    }
    
    /* Look for specific devices */
    pci_device_t* vga_dev = pci_find_class(0x03, 0x00); /* VGA controller */
    if (vga_dev) {
        vga_printf("VGA: %04X:%04X\n", vga_dev->vendor_id, vga_dev->device_id);
    }
    
    pci_device_t* net_dev = pci_find_class(0x02, 0x00); /* Ethernet */
    if (net_dev) {
        vga_printf("NET: %04X:%04X\n", net_dev->vendor_id, net_dev->device_id);
    }
    
    while (1) {
        timer_delay(5000);
    }
}

static void ata_demo_task(void) {
    ata_init();
    
    u8 drive_count = ata_get_device_count();
    
    vga_set_cursor(8, 0);
    vga_printf("ATA Drives: %d\n", drive_count);
    
    for (u8 i = 0; i < 4; i++) {
        ata_device_t* dev = ata_get_device(i);
        if (dev) {
            vga_printf("Drive %d: %s\n", i, dev->model);
            vga_printf("  Sectors: %u\n", dev->sectors);
            vga_printf("  Size: %u MB\n", dev->sectors / 2048);
            
            /* Test read first sector */
            u8 buffer[512];
            if (ata_read_sectors(i, 0, 1, buffer)) {
                vga_printf("  Read test: OK\n");
                
                /* Show first 16 bytes */
                vga_puts("  Data: ");
                for (u8 j = 0; j < 16; j++) {
                    vga_printf("%02X ", buffer[j]);
                }
                vga_puts("\n");
            } else {
                vga_printf("  Read test: FAILED\n");
            }
        }
    }
    
    while (1) {
        timer_delay(10000);
    }
}

static void timer_demo_task(void) {
    u32 last_second = 0;
    
    while (1) {
        u32 uptime = pit_get_uptime_ms() / 1000;
        
        if (uptime != last_second) {
            last_second = uptime;
            
            /* Update uptime display */
            char time_str[32];
            u32 hours = uptime / 3600;
            u32 minutes = (uptime % 3600) / 60;
            u32 seconds = uptime % 60;
            
            sprintf(time_str, "Uptime: %02u:%02u:%02u", hours, minutes, seconds);
            vga_puts_at(time_str, 0, 60, VGA_GREEN | (VGA_BLACK << 4));
            
            /* Beep every 10 seconds */
            if (seconds % 10 == 0) {
                pit_beep(1000, 100);
            }
        }
        
        timer_delay(100);
    }
}

static void cpu_info_task(void) {
    vga_set_cursor(18, 0);
    vga_puts("CPU Information:\n");

    const char* vendor = cpuid_get_vendor();
    const char* brand = cpuid_get_brand();

    vga_printf("Vendor: %s\n", vendor);
    vga_printf("Brand: %s\n", brand);
    vga_printf("Family: %u Model: %u Stepping: %u\n",
              cpuid_get_family(), cpuid_get_model(), cpuid_get_stepping());

    vga_puts("Features: ");
    if (cpuid_has_feature(CPU_FEATURE_FPU)) vga_puts("FPU ");
    if (cpuid_has_feature(CPU_FEATURE_TSC)) vga_puts("TSC ");
    if (cpuid_has_feature(CPU_FEATURE_SSE)) vga_puts("SSE ");
    if (cpuid_has_feature(CPU_FEATURE_SSE2)) vga_puts("SSE2 ");
    if (cpuid_has_feature(CPU_FEATURE_AVX)) vga_puts("AVX ");
    vga_puts("\n");

    while (1) {
        timer_delay(5000);
    }
}

static void memory_info_task(void) {
    while (1) {
        u32 free_mem = paging_get_free_memory();
        u32 used_mem = paging_get_used_memory();
        u32 total_mem = free_mem + used_mem;

        char mem_str[80];
        sprintf(mem_str, "Memory: %u/%u KB free", free_mem / 1024, total_mem / 1024);
        vga_puts_at(mem_str, 22, 0, VGA_CYAN | (VGA_BLACK << 4));

        timer_delay(2000);
    }
}

static void interrupt_test_task(void) {
    static u32 irq_counts[16] = {0};

    while (1) {
        /* Display IRQ statistics */
        char irq_str[80];
        sprintf(irq_str, "IRQ0: %u  IRQ1: %u", irq_counts[0], irq_counts[1]);
        vga_puts_at(irq_str, 21, 0, VGA_GREEN | (VGA_BLACK << 4));

        /* Test atomic operations */
        static volatile u32 atomic_test = 0;
        atomic_inc(&atomic_test);

        timer_delay(1000);
    }
}

static void rtc_demo_task(void) {
    vga_set_cursor(12, 40);
    vga_puts("Real-Time Clock:\n");

    while (1) {
        rtc_time_t time;
        rtc_read_time(&time);

        char time_str[40];
        sprintf(time_str, "%04u-%02u-%02u %02u:%02u:%02u",
                time.year, time.month, time.day,
                time.hour, time.minute, time.second);

        vga_puts_at(time_str, 13, 40, VGA_CYAN | (VGA_BLACK << 4));

        /* Show CMOS info */
        u16 base_mem = cmos_get_base_memory();
        u16 ext_mem = cmos_get_extended_memory();

        char mem_str[40];
        sprintf(mem_str, "CMOS: %u KB base, %u KB ext", base_mem, ext_mem);
        vga_puts_at(mem_str, 14, 40, VGA_LGRAY | (VGA_BLACK << 4));

        timer_delay(1000);
    }
}

static void serial_demo_task(void) {
    static u32 counter = 0;

    while (1) {
        /* Send test data to COM1 */
        serial_printf(0, "Blood Kernel COM1 test #%u\r\n", counter++);

        /* Echo any received data */
        if (serial_available(0)) {
            char c = serial_getc(0);
            serial_printf(0, "Echo: %c\r\n", c);

            /* Also display on VGA */
            char echo_str[20];
            sprintf(echo_str, "COM1 RX: %c", c);
            vga_puts_at(echo_str, 15, 40, VGA_GREEN | (VGA_BLACK << 4));
        }

        timer_delay(2000);
    }
}

static void floppy_demo_task(void) {
    vga_set_cursor(16, 40);
    vga_puts("Floppy Drives:\n");

    /* Detect floppy drives */
    for (u8 drive = 0; drive < 2; drive++) {
        u8 type = floppy_detect_type(drive);
        const floppy_geometry_t* geom = floppy_get_geometry(type);

        char drive_str[40];
        if (geom) {
            sprintf(drive_str, "FD%u: %u KB (%ux%ux%u)",
                    drive,
                    (geom->sectors_per_track * geom->heads * geom->tracks) / 2,
                    geom->tracks, geom->heads, geom->sectors_per_track);
        } else {
            sprintf(drive_str, "FD%u: Not present", drive);
        }

        vga_puts_at(drive_str, 17 + drive, 40, VGA_YELLOW | (VGA_BLACK << 4));
    }

    /* Test read from floppy A: */
    static u8 test_done = 0;
    if (!test_done) {
        u8 buffer[512];
        if (floppy_read_sector(0, 0, 0, 1, buffer)) {
            vga_puts_at("FD0: Boot sector read OK", 19, 40, VGA_GREEN | (VGA_BLACK << 4));
        } else {
            vga_puts_at("FD0: No disk or read error", 19, 40, VGA_RED | (VGA_BLACK << 4));
        }
        test_done = 1;
    }

    while (1) {
        timer_delay(5000);
    }
}

static void acpi_demo_task(void) {
    vga_set_cursor(20, 0);
    vga_puts("ACPI Information:\n");

    if (acpi_is_available()) {
        vga_puts("ACPI: Available\n");

        u8 cpu_count = acpi_get_cpu_count();
        u32 lapic_base = acpi_get_local_apic_base();

        char acpi_str[80];
        sprintf(acpi_str, "CPUs: %u  LAPIC: %08X", cpu_count, lapic_base);
        vga_puts_at(acpi_str, 21, 0, VGA_CYAN | (VGA_BLACK << 4));

        if (acpi_is_enabled()) {
            vga_puts_at("ACPI Mode: Enabled", 22, 0, VGA_GREEN | (VGA_BLACK << 4));
        } else {
            vga_puts_at("ACPI Mode: Legacy", 22, 0, VGA_YELLOW | (VGA_BLACK << 4));
        }
    } else {
        vga_puts("ACPI: Not available\n");
    }

    while (1) {
        timer_delay(5000);
    }
}

static void apic_demo_task(void) {
    if (apic_is_enabled()) {
        u8 apic_id = apic_get_id();
        u32 apic_ver = apic_get_version();

        char apic_str[40];
        sprintf(apic_str, "APIC ID: %u Ver: %08X", apic_id, apic_ver);
        vga_puts_at(apic_str, 20, 40, VGA_LBLUE | (VGA_BLACK << 4));

        /* Test APIC timer */
        apic_timer_init(100); /* 100 Hz */
        vga_puts_at("APIC Timer: 100 Hz", 21, 40, VGA_GREEN | (VGA_BLACK << 4));
    } else {
        vga_puts_at("APIC: Not available", 20, 40, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(5000);
    }
}

static void usb_demo_task(void) {
    vga_set_cursor(22, 40);
    vga_puts("USB Controllers:\n");

    u8 uhci_count = uhci_get_controller_count();

    char usb_str[40];
    sprintf(usb_str, "UHCI: %u controllers", uhci_count);
    vga_puts_at(usb_str, 23, 40, VGA_YELLOW | (VGA_BLACK << 4));

    /* Check port status */
    if (uhci_count > 0) {
        u16 port0_status = uhci_get_port_status(0, 0);
        u16 port1_status = uhci_get_port_status(0, 1);

        char port_str[40];
        sprintf(port_str, "Ports: %04X %04X", port0_status, port1_status);
        vga_puts_at(port_str, 24, 40, VGA_LGRAY | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(2000);
    }
}

static void audio_demo_task(void) {
    if (ac97_is_initialized()) {
        vga_puts_at("AC97: Initialized", 20, 0, VGA_GREEN | (VGA_BLACK << 4));

        /* Set volume to 50% */
        ac97_set_volume(50, 50);

        /* Generate test tones */
        static u32 tone_counter = 0;
        u16 frequencies[] = {440, 523, 659, 784}; /* A, C, E, G */

        if (tone_counter % 5 == 0) {
            u16 freq = frequencies[(tone_counter / 5) % 4];
            ac97_generate_tone(freq, 200);

            char tone_str[40];
            sprintf(tone_str, "Playing: %u Hz", freq);
            vga_puts_at(tone_str, 21, 0, VGA_CYAN | (VGA_BLACK << 4));
        }

        tone_counter++;
    } else {
        vga_puts_at("AC97: Not available", 20, 0, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(1000);
    }
}

static void network_demo_task(void) {
    if (rtl8139_is_initialized()) {
        vga_puts_at("RTL8139: Initialized", 22, 0, VGA_GREEN | (VGA_BLACK << 4));

        /* Get MAC address */
        u8 mac[6];
        rtl8139_get_mac_address(mac);

        char mac_str[40];
        sprintf(mac_str, "MAC: %02X:%02X:%02X:%02X:%02X:%02X",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        vga_puts_at(mac_str, 23, 0, VGA_CYAN | (VGA_BLACK << 4));

        /* Check link status */
        if (rtl8139_is_link_up()) {
            vga_puts_at("Link: UP", 24, 0, VGA_GREEN | (VGA_BLACK << 4));
        } else {
            vga_puts_at("Link: DOWN", 24, 0, VGA_RED | (VGA_BLACK << 4));
        }

        /* Send test packet */
        static u32 packet_counter = 0;
        if (packet_counter % 10 == 0) {
            u8 test_packet[64] = "Blood Kernel Network Test";
            if (rtl8139_send_packet(test_packet, sizeof(test_packet))) {
                char pkt_str[40];
                sprintf(pkt_str, "TX Packets: %u", packet_counter / 10);
                vga_puts_at(pkt_str, 24, 20, VGA_YELLOW | (VGA_BLACK << 4));
            }
        }

        packet_counter++;
    } else {
        vga_puts_at("RTL8139: Not available", 22, 0, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(1000);
    }
}

static void dma_demo_task(void) {
    vga_puts_at("DMA Controller: 8237A", 20, 40, VGA_LBLUE | (VGA_BLACK << 4));

    /* Test DMA channel availability */
    u8 available_channels = 0;
    for (u8 i = 0; i < 8; i++) {
        if (i != 4) { /* Skip cascade channel */
            extern u8 dma_is_channel_available(u8 channel);
            if (dma_is_channel_available(i)) {
                available_channels++;
            }
        }
    }

    char dma_str[40];
    sprintf(dma_str, "Available channels: %u", available_channels);
    vga_puts_at(dma_str, 21, 40, VGA_CYAN | (VGA_BLACK << 4));

    /* Show DMA status */
    extern u8 dma_get_status(u8 controller);
    u8 status1 = dma_get_status(0);
    u8 status2 = dma_get_status(1);

    char status_str[40];
    sprintf(status_str, "Status: %02X %02X", status1, status2);
    vga_puts_at(status_str, 22, 40, VGA_YELLOW | (VGA_BLACK << 4));

    while (1) {
        timer_delay(2000);
    }
}

static void hpet_demo_task(void) {
    if (hpet_is_initialized()) {
        vga_puts_at("HPET: Initialized", 20, 0, VGA_GREEN | (VGA_BLACK << 4));

        u64 frequency = hpet_get_frequency();
        u8 num_timers = hpet_get_num_timers();

        char hpet_str[40];
        sprintf(hpet_str, "Freq: %u Hz, Timers: %u", (u32)frequency, num_timers);
        vga_puts_at(hpet_str, 21, 0, VGA_CYAN | (VGA_BLACK << 4));

        /* Show high precision timestamp */
        static u32 counter = 0;
        if (counter % 10 == 0) {
            u64 timestamp = hpet_get_timestamp_us();
            char ts_str[40];
            sprintf(ts_str, "Timestamp: %u us", (u32)timestamp);
            vga_puts_at(ts_str, 22, 0, VGA_YELLOW | (VGA_BLACK << 4));
        }
        counter++;
    } else {
        vga_puts_at("HPET: Not available", 20, 0, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(100);
    }
}

static void msr_demo_task(void) {
    if (msr_is_supported()) {
        vga_puts_at("MSR: Supported", 20, 40, VGA_GREEN | (VGA_BLACK << 4));

        /* Show TSC frequency */
        u64 tsc_freq = msr_get_tsc_frequency();
        char freq_str[40];
        sprintf(freq_str, "TSC: %u MHz", (u32)(tsc_freq / 1000000));
        vga_puts_at(freq_str, 21, 40, VGA_CYAN | (VGA_BLACK << 4));

        /* Show APIC base */
        u64 apic_base = msr_get_apic_base();
        char apic_str[40];
        sprintf(apic_str, "APIC: %08X", (u32)apic_base);
        vga_puts_at(apic_str, 22, 40, VGA_YELLOW | (VGA_BLACK << 4));

        /* Show microcode version */
        u32 microcode = msr_get_microcode_version();
        char mc_str[40];
        sprintf(mc_str, "Microcode: %08X", microcode);
        vga_puts_at(mc_str, 23, 40, VGA_LGRAY | (VGA_BLACK << 4));
    } else {
        vga_puts_at("MSR: Not supported", 20, 40, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(2000);
    }
}

static void smbios_demo_task(void) {
    vga_set_cursor(12, 0);
    vga_puts("SMBIOS Information:\n");

    if (smbios_is_available()) {
        u8 major = smbios_get_version_major();
        u8 minor = smbios_get_version_minor();

        char ver_str[40];
        sprintf(ver_str, "Version: %u.%u", major, minor);
        vga_puts_at(ver_str, 13, 0, VGA_CYAN | (VGA_BLACK << 4));

        /* System information */
        const char* manufacturer = smbios_get_system_manufacturer();
        const char* product = smbios_get_system_product();

        char sys_str[80];
        sprintf(sys_str, "System: %.20s %.20s", manufacturer, product);
        vga_puts_at(sys_str, 14, 0, VGA_YELLOW | (VGA_BLACK << 4));

        /* BIOS information */
        const char* bios_vendor = smbios_get_bios_vendor();
        const char* bios_version = smbios_get_bios_version();

        char bios_str[80];
        sprintf(bios_str, "BIOS: %.15s %.15s", bios_vendor, bios_version);
        vga_puts_at(bios_str, 15, 0, VGA_GREEN | (VGA_BLACK << 4));

        /* Processor information */
        const char* cpu_mfg = smbios_get_processor_manufacturer(0);
        u16 cpu_speed = smbios_get_processor_speed(0);
        u8 cores = smbios_get_processor_core_count(0);

        char cpu_str[80];
        sprintf(cpu_str, "CPU: %.15s %u MHz %u cores", cpu_mfg, cpu_speed, cores);
        vga_puts_at(cpu_str, 16, 0, VGA_LBLUE | (VGA_BLACK << 4));
    } else {
        vga_puts_at("SMBIOS: Not available", 13, 0, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(5000);
    }
}

static void thermal_demo_task(void) {
    if (thermal_is_supported()) {
        vga_puts_at("Thermal: Supported", 20, 0, VGA_GREEN | (VGA_BLACK << 4));

        if (thermal_has_digital_sensor()) {
            u32 temperature = thermal_get_temperature();
            u32 tjmax = thermal_get_tjmax();
            u32 max_temp = thermal_get_max_temperature();

            char temp_str[40];
            sprintf(temp_str, "Temp: %u°C TjMax: %u°C", temperature, tjmax);
            vga_puts_at(temp_str, 21, 0, VGA_CYAN | (VGA_BLACK << 4));

            char max_str[40];
            sprintf(max_str, "Max: %u°C Events: %u", max_temp, thermal_get_event_count());
            vga_puts_at(max_str, 22, 0, VGA_YELLOW | (VGA_BLACK << 4));

            if (thermal_is_throttling()) {
                vga_puts_at("Status: THROTTLING", 23, 0, VGA_RED | (VGA_BLACK << 4));
            } else if (thermal_is_critical()) {
                vga_puts_at("Status: CRITICAL", 23, 0, VGA_RED | (VGA_BLACK << 4));
            } else {
                vga_puts_at("Status: Normal", 23, 0, VGA_GREEN | (VGA_BLACK << 4));
            }
        } else {
            vga_puts_at("Digital sensor: No", 21, 0, VGA_YELLOW | (VGA_BLACK << 4));
        }
    } else {
        vga_puts_at("Thermal: Not supported", 20, 0, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(2000);
    }
}

static void power_demo_task(void) {
    if (power_is_pstate_supported()) {
        vga_puts_at("P-states: Supported", 20, 40, VGA_GREEN | (VGA_BLACK << 4));

        u8 current_pstate = power_get_pstate();
        u16 frequency = power_get_frequency();
        u8 max_pstate = power_get_max_pstate();
        u8 min_pstate = power_get_min_pstate();

        char pstate_str[40];
        sprintf(pstate_str, "P%u: %u MHz (%u-%u)", current_pstate, frequency, min_pstate, max_pstate);
        vga_puts_at(pstate_str, 21, 40, VGA_CYAN | (VGA_BLACK << 4));

        if (power_is_turbo_enabled()) {
            vga_puts_at("Turbo: Enabled", 22, 40, VGA_GREEN | (VGA_BLACK << 4));
        } else {
            vga_puts_at("Turbo: Disabled", 22, 40, VGA_YELLOW | (VGA_BLACK << 4));
        }

        u8 epb = power_get_energy_perf_bias();
        char epb_str[40];
        sprintf(epb_str, "EPB: %u (0=Perf 15=Power)", epb);
        vga_puts_at(epb_str, 23, 40, VGA_LGRAY | (VGA_BLACK << 4));
    } else {
        vga_puts_at("P-states: Not supported", 20, 40, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(3000);
    }
}

static void iommu_demo_task(void) {
    vga_set_cursor(12, 40);
    vga_puts("IOMMU Information:\n");

    if (iommu_is_enabled()) {
        u8 unit_count = iommu_get_unit_count();

        char count_str[40];
        sprintf(count_str, "Units: %u", unit_count);
        vga_puts_at(count_str, 13, 40, VGA_CYAN | (VGA_BLACK << 4));

        if (unit_count > 0) {
            u8 major = iommu_get_unit_version_major(0);
            u8 minor = iommu_get_unit_version_minor(0);
            u32 caps = iommu_get_unit_capabilities(0);

            char ver_str[40];
            sprintf(ver_str, "Version: %u.%u", major, minor);
            vga_puts_at(ver_str, 14, 40, VGA_YELLOW | (VGA_BLACK << 4));

            char caps_str[40];
            sprintf(caps_str, "Caps: %08X", caps);
            vga_puts_at(caps_str, 15, 40, VGA_LGRAY | (VGA_BLACK << 4));

            vga_puts_at("Status: Active", 16, 40, VGA_GREEN | (VGA_BLACK << 4));
        }
    } else {
        vga_puts_at("IOMMU: Not available", 13, 40, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(5000);
    }
}

static void cache_demo_task(void) {
    if (cache_is_mtrr_supported()) {
        vga_puts_at("Cache: MTRR Supported", 20, 0, VGA_GREEN | (VGA_BLACK << 4));

        u8 num_mtrrs = cache_get_num_variable_mtrrs();
        u32 line_size = cache_get_line_size();
        u32 l1_size = cache_get_l1_size();
        u32 l2_size = cache_get_l2_size();

        char mtrr_str[40];
        sprintf(mtrr_str, "MTRRs: %u Line: %u bytes", num_mtrrs, line_size);
        vga_puts_at(mtrr_str, 21, 0, VGA_CYAN | (VGA_BLACK << 4));

        char cache_str[40];
        sprintf(cache_str, "L1: %uKB L2: %uKB", l1_size, l2_size);
        vga_puts_at(cache_str, 22, 0, VGA_YELLOW | (VGA_BLACK << 4));

        if (cache_is_pat_supported()) {
            vga_puts_at("PAT: Supported", 23, 0, VGA_GREEN | (VGA_BLACK << 4));
        } else {
            vga_puts_at("PAT: Not supported", 23, 0, VGA_RED | (VGA_BLACK << 4));
        }
    } else {
        vga_puts_at("Cache: MTRR Not supported", 20, 0, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(3000);
    }
}

static void vmx_demo_task(void) {
    if (vmx_is_supported()) {
        vga_puts_at("VMX: Supported", 20, 40, VGA_GREEN | (VGA_BLACK << 4));

        if (vmx_is_enabled()) {
            vga_puts_at("Status: Enabled", 21, 40, VGA_GREEN | (VGA_BLACK << 4));

            u32 vmcs_rev = vmx_get_vmcs_revision_id();
            u32 vmcs_size = vmx_get_vmcs_size();

            char vmcs_str[40];
            sprintf(vmcs_str, "VMCS: Rev %08X Size %u", vmcs_rev, vmcs_size);
            vga_puts_at(vmcs_str, 22, 40, VGA_CYAN | (VGA_BLACK << 4));

            char features[40] = "Features: ";
            if (vmx_is_ept_supported()) strcat(features, "EPT ");
            if (vmx_is_vpid_supported()) strcat(features, "VPID ");
            if (vmx_is_unrestricted_guest_supported()) strcat(features, "UG");
            vga_puts_at(features, 23, 40, VGA_YELLOW | (VGA_BLACK << 4));
        } else {
            vga_puts_at("Status: Disabled", 21, 40, VGA_YELLOW | (VGA_BLACK << 4));
        }
    } else {
        vga_puts_at("VMX: Not supported", 20, 40, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(4000);
    }
}

static void perfmon_demo_task(void) {
    vga_set_cursor(12, 0);
    vga_puts("Performance Monitoring:\n");

    if (perfmon_is_supported()) {
        u8 version = perfmon_get_version();
        u8 num_counters = perfmon_get_num_counters();
        u8 counter_width = perfmon_get_counter_width();
        u8 num_fixed = perfmon_get_num_fixed_counters();

        char ver_str[40];
        sprintf(ver_str, "Version: %u Counters: %u/%u", version, num_counters, num_fixed);
        vga_puts_at(ver_str, 13, 0, VGA_CYAN | (VGA_BLACK << 4));

        char width_str[40];
        sprintf(width_str, "Width: %u bits", counter_width);
        vga_puts_at(width_str, 14, 0, VGA_YELLOW | (VGA_BLACK << 4));

        /* Setup and enable instruction counter */
        static u8 setup_done = 0;
        if (!setup_done) {
            perfmon_setup_counter(0, PERF_EVENT_INSTRUCTIONS, 0, 1, 1, "Instructions");
            perfmon_enable_counter(0);
            perfmon_enable_fixed_counter(0); /* Instructions retired */
            perfmon_enable_all();
            setup_done = 1;
        }

        /* Read counters */
        u64 instructions = perfmon_read_counter(0);
        u64 fixed_instructions = perfmon_read_fixed_counter(0);

        char inst_str[40];
        sprintf(inst_str, "Instructions: %u", (u32)instructions);
        vga_puts_at(inst_str, 15, 0, VGA_GREEN | (VGA_BLACK << 4));

        char fixed_str[40];
        sprintf(fixed_str, "Fixed: %u", (u32)fixed_instructions);
        vga_puts_at(fixed_str, 16, 0, VGA_LBLUE | (VGA_BLACK << 4));
    } else {
        vga_puts_at("PerfMon: Not supported", 13, 0, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(2000);
    }
}

static void longmode_demo_task(void) {
    if (longmode_is_supported()) {
        vga_puts_at("LongMode: Supported", 20, 0, VGA_GREEN | (VGA_BLACK << 4));

        if (longmode_is_enabled()) {
            vga_puts_at("Status: Enabled", 21, 0, VGA_GREEN | (VGA_BLACK << 4));

            char features[40] = "Features: ";
            if (longmode_is_nx_supported()) strcat(features, "NX ");
            if (longmode_is_syscall_supported()) strcat(features, "SYSCALL ");
            if (longmode_is_pae_enabled()) strcat(features, "PAE");
            vga_puts_at(features, 22, 0, VGA_CYAN | (VGA_BLACK << 4));

            u64 efer = longmode_get_efer();
            char efer_str[40];
            sprintf(efer_str, "EFER: %08X%08X", (u32)(efer >> 32), (u32)efer);
            vga_puts_at(efer_str, 23, 0, VGA_YELLOW | (VGA_BLACK << 4));
        } else {
            vga_puts_at("Status: Disabled", 21, 0, VGA_YELLOW | (VGA_BLACK << 4));
        }
    } else {
        vga_puts_at("LongMode: Not supported", 20, 0, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(3000);
    }
}

static void microcode_demo_task(void) {
    if (microcode_is_supported()) {
        vga_puts_at("Microcode: Supported", 20, 40, VGA_GREEN | (VGA_BLACK << 4));

        const char* vendor = microcode_is_intel() ? "Intel" :
                           microcode_is_amd() ? "AMD" : "Unknown";
        char vendor_str[40];
        sprintf(vendor_str, "Vendor: %s", vendor);
        vga_puts_at(vendor_str, 21, 40, VGA_CYAN | (VGA_BLACK << 4));

        u32 revision = microcode_get_revision();
        u32 signature = microcode_get_processor_signature();

        char rev_str[40];
        sprintf(rev_str, "Revision: %08X", revision);
        vga_puts_at(rev_str, 22, 40, VGA_YELLOW | (VGA_BLACK << 4));

        char sig_str[40];
        sprintf(sig_str, "Signature: %08X", signature);
        vga_puts_at(sig_str, 23, 40, VGA_LGRAY | (VGA_BLACK << 4));
    } else {
        vga_puts_at("Microcode: Not supported", 20, 40, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(4000);
    }
}

static void x2apic_demo_task(void) {
    vga_set_cursor(12, 40);
    vga_puts("x2APIC Information:\n");

    if (x2apic_is_supported()) {
        char support_str[40];
        sprintf(support_str, "Supported: Yes");
        vga_puts_at(support_str, 13, 40, VGA_GREEN | (VGA_BLACK << 4));

        if (x2apic_is_enabled()) {
            vga_puts_at("Status: Enabled", 14, 40, VGA_GREEN | (VGA_BLACK << 4));

            u32 apic_id = x2apic_get_id();
            u32 version = x2apic_get_version();
            u32 max_lvt = x2apic_get_max_lvt_entries();

            char id_str[40];
            sprintf(id_str, "ID: %08X Ver: %u", apic_id, version);
            vga_puts_at(id_str, 15, 40, VGA_CYAN | (VGA_BLACK << 4));

            char lvt_str[40];
            sprintf(lvt_str, "Max LVT: %u", max_lvt);
            vga_puts_at(lvt_str, 16, 40, VGA_YELLOW | (VGA_BLACK << 4));

            if (x2apic_supports_eoi_broadcast_suppression()) {
                vga_puts_at("EOI Broadcast: Suppressed", 17, 40, VGA_LBLUE | (VGA_BLACK << 4));
            }
        } else {
            vga_puts_at("Status: Disabled", 14, 40, VGA_YELLOW | (VGA_BLACK << 4));
        }
    } else {
        vga_puts_at("x2APIC: Not supported", 13, 40, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(5000);
    }
}

static void topology_demo_task(void) {
    if (topology_is_supported()) {
        vga_puts_at("Topology: Supported", 20, 0, VGA_GREEN | (VGA_BLACK << 4));

        u8 packages = topology_get_num_packages();
        u8 cores = topology_get_num_cores_per_package();
        u8 threads = topology_get_num_threads_per_core();
        u8 logical = topology_get_num_logical_processors();

        char topo_str[40];
        sprintf(topo_str, "P:%u C:%u T:%u L:%u", packages, cores, threads, logical);
        vga_puts_at(topo_str, 21, 0, VGA_CYAN | (VGA_BLACK << 4));

        u8 pkg_id = topology_get_current_package_id();
        u8 core_id = topology_get_current_core_id();
        u8 thread_id = topology_get_current_thread_id();

        char current_str[40];
        sprintf(current_str, "Current: P%u C%u T%u", pkg_id, core_id, thread_id);
        vga_puts_at(current_str, 22, 0, VGA_YELLOW | (VGA_BLACK << 4));

        if (topology_is_numa_supported()) {
            u8 numa_nodes = topology_get_num_numa_nodes();
            u8 current_node = topology_get_current_numa_node();

            char numa_str[40];
            sprintf(numa_str, "NUMA: %u nodes, current %u", numa_nodes, current_node);
            vga_puts_at(numa_str, 23, 0, VGA_LBLUE | (VGA_BLACK << 4));
        } else {
            vga_puts_at("NUMA: Not supported", 23, 0, VGA_RED | (VGA_BLACK << 4));
        }
    } else {
        vga_puts_at("Topology: Not supported", 20, 0, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(3000);
    }
}

static void xsave_demo_task(void) {
    if (xsave_is_supported()) {
        vga_puts_at("XSAVE: Supported", 20, 40, VGA_GREEN | (VGA_BLACK << 4));

        u64 supported = xsave_get_supported_features();
        u64 enabled = xsave_get_enabled_features();
        u32 area_size = xsave_get_area_size();

        char features_str[40];
        sprintf(features_str, "Features: %04X/%04X", (u32)enabled, (u32)supported);
        vga_puts_at(features_str, 21, 40, VGA_CYAN | (VGA_BLACK << 4));

        char size_str[40];
        sprintf(size_str, "Area size: %u bytes", area_size);
        vga_puts_at(size_str, 22, 40, VGA_YELLOW | (VGA_BLACK << 4));

        char variants[40] = "Variants: ";
        if (xsave_is_xsaveopt_supported()) strcat(variants, "OPT ");
        if (xsave_is_xsavec_supported()) strcat(variants, "C ");
        if (xsave_is_xsaves_supported()) strcat(variants, "S");
        vga_puts_at(variants, 23, 40, VGA_LGRAY | (VGA_BLACK << 4));
    } else {
        vga_puts_at("XSAVE: Not supported", 20, 40, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(4000);
    }
}

static void numa_demo_task(void) {
    vga_set_cursor(12, 0);
    vga_puts("NUMA Information:\n");

    if (numa_is_enabled()) {
        u8 num_nodes = numa_get_num_nodes();
        u8 current_node = numa_get_current_node();
        u32 policy = numa_get_allocation_policy();

        char nodes_str[40];
        sprintf(nodes_str, "Nodes: %u Current: %u", num_nodes, current_node);
        vga_puts_at(nodes_str, 13, 0, VGA_CYAN | (VGA_BLACK << 4));

        const char* policy_names[] = {"Default", "Bind", "Interleave", "Preferred"};
        char policy_str[40];
        sprintf(policy_str, "Policy: %s", policy_names[policy % 4]);
        vga_puts_at(policy_str, 14, 0, VGA_YELLOW | (VGA_BLACK << 4));

        if (num_nodes > 0) {
            u64 node0_mem = numa_get_node_memory_size(0);
            u64 node0_free = numa_get_node_free_memory(0);

            char mem_str[40];
            sprintf(mem_str, "Node0: %uMB/%uMB", (u32)(node0_free >> 20), (u32)(node0_mem >> 20));
            vga_puts_at(mem_str, 15, 0, VGA_GREEN | (VGA_BLACK << 4));

            if (num_nodes > 1) {
                u8 distance = numa_get_distance(0, 1);
                char dist_str[40];
                sprintf(dist_str, "Distance 0->1: %u", distance);
                vga_puts_at(dist_str, 16, 0, VGA_LBLUE | (VGA_BLACK << 4));
            }
        }
    } else {
        vga_puts_at("NUMA: Disabled", 13, 0, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(5000);
    }
}

static void system_info_task(void) {
    while (1) {
        /* Update system stats */
        u32 ticks = pit_get_ticks();
        u32 freq = pit_get_frequency();

        char stats[80];
        sprintf(stats, "Ticks: %u  Freq: %u Hz", ticks, freq);
        vga_puts_at(stats, 24, 0, VGA_YELLOW | (VGA_BLACK << 4));

        timer_delay(1000);
    }
}

void x86_pc_demo_init(void) {
    /* Called from kernel_main for x86 */
    extern void task_create(void (*func)(void), u8 prio, u16 stack_size);
    
    /* Create demo tasks */
    task_create(vga_demo_task, 1, 512);
    task_create(keyboard_demo_task, 2, 256);
    task_create(pci_demo_task, 3, 512);
    task_create(ata_demo_task, 4, 512);
    task_create(timer_demo_task, 5, 256);
    task_create(cpu_info_task, 6, 256);
    task_create(memory_info_task, 7, 256);
    task_create(interrupt_test_task, 8, 256);
    task_create(rtc_demo_task, 9, 256);
    task_create(serial_demo_task, 10, 256);
    task_create(floppy_demo_task, 11, 512);
    task_create(smbios_demo_task, 12, 256);
    task_create(numa_demo_task, 13, 256);
    task_create(perfmon_demo_task, 14, 256);
    task_create(x2apic_demo_task, 15, 256);
    task_create(iommu_demo_task, 16, 256);
    task_create(acpi_demo_task, 17, 256);
    task_create(apic_demo_task, 18, 256);
    task_create(usb_demo_task, 19, 256);
    task_create(audio_demo_task, 20, 256);
    task_create(network_demo_task, 21, 256);
    task_create(dma_demo_task, 22, 256);
    task_create(hpet_demo_task, 23, 256);
    task_create(msr_demo_task, 24, 256);
    task_create(thermal_demo_task, 25, 256);
    task_create(power_demo_task, 26, 256);
    task_create(cache_demo_task, 27, 256);
    task_create(vmx_demo_task, 28, 256);
    task_create(longmode_demo_task, 29, 256);
    task_create(microcode_demo_task, 30, 256);
    task_create(topology_demo_task, 31, 256);
    task_create(xsave_demo_task, 32, 256);
    task_create(system_info_task, 33, 256);
}

/* Simple string functions */
char* strcat(char* dest, const char* src) {
    char* d = dest;
    while (*d) d++;
    while ((*d++ = *src++));
    return dest;
}

int sprintf(char* str, const char* fmt, ...) {
    /* Simplified sprintf */
    const char* p = fmt;
    char* s = str;
    
    while (*p) {
        if (*p == '%' && *(p + 1)) {
            p++;
            switch (*p) {
                case 'd':
                case 'u': {
                    *s++ = '4';
                    *s++ = '2';
                    break;
                }
                case 'X':
                case 'x': {
                    *s++ = 'F';
                    *s++ = 'F';
                    break;
                }
                case 's': {
                    *s++ = 's';
                    *s++ = 't';
                    *s++ = 'r';
                    break;
                }
                default:
                    *s++ = *p;
                    break;
            }
        } else {
            *s++ = *p;
        }
        p++;
    }
    *s = 0;
    return s - str;
}
