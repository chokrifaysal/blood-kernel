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
#include "drivers/security.h"
#include "drivers/debug.h"
#include "drivers/errata.h"
#include "drivers/cpufreq.h"
#include "drivers/ioapic.h"
#include "drivers/cpuid_ext.h"
#include "drivers/cache_mgmt.h"
#include "drivers/memory_mgmt.h"
#include "drivers/cpu_features.h"
#include "drivers/simd.h"
#include "drivers/interrupt_mgmt.h"
#include "drivers/atomic.h"
#include "drivers/power_mgmt.h"
#include "drivers/memory_protection.h"
#include "drivers/virt_ext.h"
#include "drivers/debug_hw.h"
#include "drivers/pmu_ext.h"
#include "drivers/trace_hw.h"
#include "drivers/exception_mgmt.h"
#include "drivers/memory_adv.h"
#include "drivers/security_ext.h"
#include "drivers/freq_scaling.h"
#include "drivers/system_ctrl.h"
#include "drivers/cpu_ext.h"
#include "drivers/microarch.h"
#include "drivers/apic_ext.h"
#include "drivers/timing_sync.h"
#include "drivers/cache_coherency.h"
#include "drivers/memory_ordering.h"
#include "drivers/power_states.h"
#include "drivers/cpu_topology.h"
#include "drivers/interrupt_routing.h"
#include "drivers/cpu_errata.h"
#include "drivers/virtualization.h"
#include "drivers/iommu.h"
#include "drivers/performance_monitoring.h"
#include "drivers/cpu_security.h"
#include "drivers/memory_protection_adv.h"
#include "drivers/cpu_debug_ext.h"
#include "drivers/cpu_instruction_ext.h"
#include "drivers/system_mgmt_mode.h"
#include "drivers/thermal_mgmt.h"
#include "drivers/hwcrypto.h"

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
    vga_puts("- Microarch: Branch pred/spec\n");
    vga_puts("- APIC Ext: x2APIC/IPI/Timer\n");
    vga_puts("- Timing: TSC/HPET/Sync\n");
    vga_puts("- InstExt: AVX-512/AMX/APX\n");
    vga_puts("- SMM: System Management Mode\n");
    vga_puts("- Thermal: RAPL/Throttling\n");
    vga_puts("- Security: CET/MPX/SMEP/SMAP\n");
    vga_puts("- MemProt: PKU/NX/Domains\n");
    vga_puts("- Debug: Intel PT/LBR/BTS\n");
    vga_puts("- Virtualization: VMX/SVM\n");
    vga_puts("- IOMMU: VT-d/AMD-Vi DMA\n");
    vga_puts("- PerfMon: PMC/PMU counters\n");
    vga_puts("- Topology: Cores/Threads/NUMA\n");
    vga_puts("- Interrupt: IOAPIC/MSI/MSI-X\n");
    vga_puts("- Errata: Microcode/Workarounds\n");
    vga_puts("- Cache: MESI/MOESI coherency\n");
    vga_puts("- Memory: Ordering/Barriers\n");
    vga_puts("- Power: C-states/P-states\n");
    vga_puts("- Microarch: Branch pred/spec\n");
    vga_puts("- APIC Ext: x2APIC/IPI/Timer\n");
    vga_puts("- Timing: TSC/HPET/Sync\n");
    vga_puts("- Freq Scaling: P-states/Turbo\n");
    vga_puts("- System Ctrl: SMM/ACPI/Reset\n");
    vga_puts("- CPU Ext: AVX-512/AMX/XSAVE\n");
    vga_puts("- Exception: Fault mgmt/recovery\n");
    vga_puts("- Memory Adv: PCID/INVPCID\n");
    vga_puts("- Security: MPX/CET extensions\n");
    vga_puts("- Debug: HW breakpoints/trace\n");
    vga_puts("- PMU: Performance monitoring\n");
    vga_puts("- Trace: Intel PT/BTS/LBR\n");
    vga_puts("- Power: C-states/P-states mgmt\n");
    vga_puts("- Protection: SMEP/SMAP/CET\n");
    vga_puts("- Virtualization: VT-x/EPT\n");
    vga_puts("- SIMD: SSE/AVX optimization\n");
    vga_puts("- Interrupt: NMI/SMI/MCE mgmt\n");
    vga_puts("- Atomic: Sync primitives\n");
    vga_puts("- Cache: L1/L2/L3 management\n");
    vga_puts("- Memory: PAT/MTRR extensions\n");
    vga_puts("- Features: CPU control/config\n");
    vga_puts("- CPUFreq: SpeedStep/Cool'n'Quiet\n");
    vga_puts("- IOAPIC: Advanced interrupt ctrl\n");
    vga_puts("- CPUID: Extended feature detect\n");
    vga_puts("- Security: CET/SMEP/SMAP/MPX\n");
    vga_puts("- Debug: LBR/BTS branch tracing\n");
    vga_puts("- Errata: CPU bug workarounds\n");
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

static void security_demo_task(void) {
    vga_puts_at("Security Features:", 20, 0, VGA_WHITE | (VGA_BLACK << 4));

    char features[40] = "Features: ";
    if (security_is_smep_supported()) strcat(features, "SMEP ");
    if (security_is_smap_supported()) strcat(features, "SMAP ");
    if (security_is_cet_supported()) strcat(features, "CET ");
    if (security_is_mpx_supported()) strcat(features, "MPX ");
    if (security_is_pku_supported()) strcat(features, "PKU");
    vga_puts_at(features, 21, 0, VGA_CYAN | (VGA_BLACK << 4));

    char enabled[40] = "Enabled: ";
    if (security_is_smep_enabled()) strcat(enabled, "SMEP ");
    if (security_is_smap_enabled()) strcat(enabled, "SMAP ");
    if (security_is_cet_enabled()) strcat(enabled, "CET ");
    if (security_is_mpx_enabled()) strcat(enabled, "MPX ");
    if (security_is_pku_enabled()) strcat(enabled, "PKU");
    vga_puts_at(enabled, 22, 0, VGA_GREEN | (VGA_BLACK << 4));

    if (security_is_cet_supported()) {
        char cet_info[40] = "CET: ";
        if (security_is_shadow_stack_supported()) strcat(cet_info, "SS ");
        if (security_is_ibt_supported()) strcat(cet_info, "IBT");
        vga_puts_at(cet_info, 23, 0, VGA_YELLOW | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(3000);
    }
}

static void debug_demo_task(void) {
    vga_puts_at("Debug Features:", 20, 40, VGA_WHITE | (VGA_BLACK << 4));

    if (debug_is_lbr_supported()) {
        u8 depth = debug_get_lbr_depth();
        u8 format = debug_get_lbr_format();

        char lbr_str[40];
        sprintf(lbr_str, "LBR: Depth %u Format %u", depth, format);
        vga_puts_at(lbr_str, 21, 40, VGA_CYAN | (VGA_BLACK << 4));

        if (debug_is_lbr_enabled()) {
            vga_puts_at("LBR: Enabled", 22, 40, VGA_GREEN | (VGA_BLACK << 4));
        } else {
            vga_puts_at("LBR: Disabled", 22, 40, VGA_YELLOW | (VGA_BLACK << 4));
        }
    } else {
        vga_puts_at("LBR: Not supported", 21, 40, VGA_RED | (VGA_BLACK << 4));
    }

    if (debug_is_bts_supported()) {
        u32 buffer_size = debug_get_bts_buffer_size();

        char bts_str[40];
        sprintf(bts_str, "BTS: Buffer %u entries", buffer_size);
        vga_puts_at(bts_str, 23, 40, VGA_LBLUE | (VGA_BLACK << 4));
    } else {
        vga_puts_at("BTS: Not supported", 23, 40, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(4000);
    }
}

static void errata_demo_task(void) {
    vga_set_cursor(12, 0);
    vga_puts("CPU Errata Information:\n");

    u32 signature = errata_get_cpu_signature();
    u32 family = errata_get_family();
    u32 model = errata_get_model();
    u32 stepping = errata_get_stepping();

    char cpu_str[40];
    sprintf(cpu_str, "CPU: F%02X M%02X S%02X", family, model, stepping);
    vga_puts_at(cpu_str, 13, 0, VGA_CYAN | (VGA_BLACK << 4));

    char sig_str[40];
    sprintf(sig_str, "Signature: %08X", signature);
    vga_puts_at(sig_str, 14, 0, VGA_YELLOW | (VGA_BLACK << 4));

    const char* vendor = errata_is_intel() ? "Intel" :
                        errata_is_amd() ? "AMD" : "Unknown";
    char vendor_str[40];
    sprintf(vendor_str, "Vendor: %s", vendor);
    vga_puts_at(vendor_str, 15, 0, VGA_LGRAY | (VGA_BLACK << 4));

    u32 errata_count = errata_get_active_count();
    char count_str[40];
    sprintf(count_str, "Active errata: %u", errata_count);
    vga_puts_at(count_str, 16, 0, VGA_GREEN | (VGA_BLACK << 4));

    if (errata_count > 0) {
        u16 first_id = errata_get_active_id(0);
        const char* desc = errata_get_description(first_id);

        char first_str[40];
        sprintf(first_str, "First: %u", first_id);
        vga_puts_at(first_str, 17, 0, VGA_LBLUE | (VGA_BLACK << 4));

        /* Show first few characters of description */
        char desc_str[40];
        sprintf(desc_str, "%.30s", desc);
        vga_puts_at(desc_str, 18, 0, VGA_WHITE | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(5000);
    }
}

static void cpufreq_demo_task(void) {
    vga_puts_at("CPU Frequency Scaling:", 20, 0, VGA_WHITE | (VGA_BLACK << 4));

    if (cpufreq_is_supported()) {
        const char* tech = cpufreq_is_intel_speedstep() ? "SpeedStep" :
                          cpufreq_is_amd_coolnquiet() ? "Cool'n'Quiet" : "Unknown";

        char tech_str[40];
        sprintf(tech_str, "Technology: %s", tech);
        vga_puts_at(tech_str, 21, 0, VGA_CYAN | (VGA_BLACK << 4));

        u16 current_freq = cpufreq_get_current_frequency();
        u16 base_freq = cpufreq_get_base_frequency();
        u16 max_freq = cpufreq_get_max_turbo_frequency();

        char freq_str[40];
        sprintf(freq_str, "Freq: %uMHz/%uMHz/%uMHz", current_freq, base_freq, max_freq);
        vga_puts_at(freq_str, 22, 0, VGA_YELLOW | (VGA_BLACK << 4));

        u8 pstates = cpufreq_get_num_pstates();
        u8 current_pstate = cpufreq_get_current_pstate();

        char pstate_str[40];
        sprintf(pstate_str, "P-states: %u/%u", current_pstate, pstates);
        vga_puts_at(pstate_str, 23, 0, VGA_GREEN | (VGA_BLACK << 4));

        if (cpufreq_is_turbo_supported()) {
            const char* turbo_status = cpufreq_is_turbo_enabled() ? "Enabled" : "Disabled";
            char turbo_str[40];
            sprintf(turbo_str, "Turbo: %s", turbo_status);
            vga_puts_at(turbo_str, 24, 0, VGA_LBLUE | (VGA_BLACK << 4));
        }
    } else {
        vga_puts_at("CPU frequency scaling not supported", 21, 0, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(3000);
    }
}

static void ioapic_demo_task(void) {
    vga_puts_at("IOAPIC Information:", 20, 40, VGA_WHITE | (VGA_BLACK << 4));

    if (ioapic_is_initialized()) {
        u8 num_controllers = ioapic_get_num_controllers();

        char ctrl_str[40];
        sprintf(ctrl_str, "Controllers: %u", num_controllers);
        vga_puts_at(ctrl_str, 21, 40, VGA_CYAN | (VGA_BLACK << 4));

        if (num_controllers > 0) {
            u32 base = ioapic_get_controller_base(0);
            u8 id = ioapic_get_controller_id(0);
            u8 max_entries = ioapic_get_max_redirection_entries(0);

            char base_str[40];
            sprintf(base_str, "Base: %08X ID: %u", base, id);
            vga_puts_at(base_str, 22, 40, VGA_YELLOW | (VGA_BLACK << 4));

            char entries_str[40];
            sprintf(entries_str, "Max entries: %u", max_entries);
            vga_puts_at(entries_str, 23, 40, VGA_GREEN | (VGA_BLACK << 4));

            u32 gsi_base = ioapic_get_gsi_base(0);
            char gsi_str[40];
            sprintf(gsi_str, "GSI base: %u", gsi_base);
            vga_puts_at(gsi_str, 24, 40, VGA_LGRAY | (VGA_BLACK << 4));
        }
    } else {
        vga_puts_at("IOAPIC not initialized", 21, 40, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(4000);
    }
}

static void cpuid_ext_demo_task(void) {
    vga_set_cursor(12, 40);
    vga_puts("Extended CPUID Features:\n");

    const char* vendor = cpuid_ext_get_vendor_string();
    const char* brand = cpuid_ext_get_brand_string();

    char vendor_str[40];
    sprintf(vendor_str, "Vendor: %.12s", vendor);
    vga_puts_at(vendor_str, 13, 40, VGA_CYAN | (VGA_BLACK << 4));

    /* Show first part of brand string */
    char brand_str[40];
    sprintf(brand_str, "%.30s", brand);
    vga_puts_at(brand_str, 14, 40, VGA_YELLOW | (VGA_BLACK << 4));

    u8 family = cpuid_ext_get_family();
    u8 model = cpuid_ext_get_model();
    u8 stepping = cpuid_ext_get_stepping();

    char fms_str[40];
    sprintf(fms_str, "F%02X M%02X S%02X", family, model, stepping);
    vga_puts_at(fms_str, 15, 40, VGA_GREEN | (VGA_BLACK << 4));

    u8 cache_line = cpuid_ext_get_cache_line_size();
    u8 max_logical = cpuid_ext_get_max_logical_processors();

    char cache_str[40];
    sprintf(cache_str, "Cache: %uB Logical: %u", cache_line, max_logical);
    vga_puts_at(cache_str, 16, 40, VGA_LBLUE | (VGA_BLACK << 4));

    const cpuid_features_t* features = cpuid_ext_get_features();
    char feat_str[40] = "Features: ";
    if (features->avx) strcat(feat_str, "AVX ");
    if (features->avx2) strcat(feat_str, "AVX2 ");
    if (features->avx512f) strcat(feat_str, "AVX512");
    vga_puts_at(feat_str, 17, 40, VGA_WHITE | (VGA_BLACK << 4));

    while (1) {
        timer_delay(5000);
    }
}

static void cache_mgmt_demo_task(void) {
    vga_puts_at("Cache Management:", 20, 0, VGA_WHITE | (VGA_BLACK << 4));

    if (cache_mgmt_is_supported()) {
        u8 levels = cache_mgmt_get_num_levels();
        u32 total_size = cache_mgmt_get_total_cache_size();

        char levels_str[40];
        sprintf(levels_str, "Levels: %u Total: %uKB", levels, total_size / 1024);
        vga_puts_at(levels_str, 21, 0, VGA_CYAN | (VGA_BLACK << 4));

        /* Show L1 cache info */
        u32 l1d_size = cache_mgmt_get_cache_size(1, CACHE_TYPE_DATA);
        u16 l1d_line = cache_mgmt_get_line_size(1, CACHE_TYPE_DATA);

        if (l1d_size > 0) {
            char l1_str[40];
            sprintf(l1_str, "L1D: %uKB/%uB", l1d_size / 1024, l1d_line);
            vga_puts_at(l1_str, 22, 0, VGA_YELLOW | (VGA_BLACK << 4));
        }

        /* Show L2 cache info */
        u32 l2_size = cache_mgmt_get_cache_size(2, CACHE_TYPE_UNIFIED);
        if (l2_size > 0) {
            char l2_str[40];
            sprintf(l2_str, "L2: %uKB", l2_size / 1024);
            vga_puts_at(l2_str, 23, 0, VGA_GREEN | (VGA_BLACK << 4));
        }

        /* Show features */
        char features[40] = "Features: ";
        if (cache_mgmt_is_clflush_supported()) strcat(features, "CLFLUSH ");
        if (cache_mgmt_is_clflushopt_supported()) strcat(features, "OPT ");
        if (cache_mgmt_is_clwb_supported()) strcat(features, "CLWB");
        vga_puts_at(features, 24, 0, VGA_LBLUE | (VGA_BLACK << 4));
    } else {
        vga_puts_at("Cache management not supported", 21, 0, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(3000);
    }
}

static void memory_mgmt_demo_task(void) {
    vga_puts_at("Memory Management:", 20, 40, VGA_WHITE | (VGA_BLACK << 4));

    if (memory_mgmt_is_supported()) {
        char support_str[40] = "Support: ";
        if (memory_mgmt_is_pat_supported()) strcat(support_str, "PAT ");
        if (memory_mgmt_is_mtrr_supported()) strcat(support_str, "MTRR ");
        if (memory_mgmt_is_wc_supported()) strcat(support_str, "WC");
        vga_puts_at(support_str, 21, 40, VGA_CYAN | (VGA_BLACK << 4));

        if (memory_mgmt_is_mtrr_supported()) {
            u8 num_mtrrs = memory_mgmt_get_num_variable_mtrrs();
            u8 default_type = memory_mgmt_get_default_memory_type();

            char mtrr_str[40];
            sprintf(mtrr_str, "MTRRs: %u Default: %u", num_mtrrs, default_type);
            vga_puts_at(mtrr_str, 22, 40, VGA_YELLOW | (VGA_BLACK << 4));
        }

        if (memory_mgmt_is_pat_supported()) {
            u64 pat_value = memory_mgmt_get_pat_value();
            char pat_str[40];
            sprintf(pat_str, "PAT: %08X%08X", (u32)(pat_value >> 32), (u32)pat_value);
            vga_puts_at(pat_str, 23, 40, VGA_GREEN | (VGA_BLACK << 4));
        }

        char advanced[40] = "Advanced: ";
        if (memory_mgmt_is_memory_encryption_supported()) strcat(advanced, "SME ");
        if (memory_mgmt_is_protection_keys_supported()) strcat(advanced, "PKU ");
        if (memory_mgmt_is_smrr_supported()) strcat(advanced, "SMRR");
        vga_puts_at(advanced, 24, 40, VGA_LGRAY | (VGA_BLACK << 4));
    } else {
        vga_puts_at("Memory management not supported", 21, 40, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(4000);
    }
}

static void cpu_features_demo_task(void) {
    vga_set_cursor(12, 0);
    vga_puts("CPU Features Control:\n");

    const char* vendor = cpu_features_is_intel() ? "Intel" :
                        cpu_features_is_amd() ? "AMD" : "Unknown";

    char vendor_str[40];
    sprintf(vendor_str, "Vendor: %s", vendor);
    vga_puts_at(vendor_str, 13, 0, VGA_CYAN | (VGA_BLACK << 4));

    char vmx_str[40] = "VMX: ";
    if (cpu_features_is_vmx_supported()) {
        strcat(vmx_str, cpu_features_is_vmx_locked() ? "Locked" : "Available");
    } else {
        strcat(vmx_str, "Not supported");
    }
    vga_puts_at(vmx_str, 14, 0, VGA_YELLOW | (VGA_BLACK << 4));

    char speedstep_str[40] = "SpeedStep: ";
    strcat(speedstep_str, cpu_features_is_speedstep_enabled() ? "Enabled" : "Disabled");
    vga_puts_at(speedstep_str, 15, 0, VGA_GREEN | (VGA_BLACK << 4));

    char turbo_str[40] = "Turbo: ";
    strcat(turbo_str, cpu_features_is_turbo_enabled() ? "Enabled" : "Disabled");
    vga_puts_at(turbo_str, 16, 0, VGA_LBLUE | (VGA_BLACK << 4));

    if (cpu_features_is_intel()) {
        u32 max_ratio = cpu_features_get_max_non_turbo_ratio();
        u32 turbo_ratio = cpu_features_get_max_turbo_ratio();

        char ratio_str[40];
        sprintf(ratio_str, "Ratios: %u/%u", max_ratio, turbo_ratio);
        vga_puts_at(ratio_str, 17, 0, VGA_WHITE | (VGA_BLACK << 4));
    }

    char xd_str[40] = "XD: ";
    strcat(xd_str, cpu_features_is_xd_enabled() ? "Enabled" : "Disabled");
    vga_puts_at(xd_str, 18, 0, VGA_LGRAY | (VGA_BLACK << 4));

    while (1) {
        timer_delay(5000);
    }
}

static void simd_demo_task(void) {
    vga_puts_at("SIMD Optimization:", 20, 0, VGA_WHITE | (VGA_BLACK << 4));

    if (simd_is_optimization_enabled()) {
        u32 vector_width = simd_get_vector_width();

        char width_str[40];
        sprintf(width_str, "Vector width: %u bits", vector_width * 8);
        vga_puts_at(width_str, 21, 0, VGA_CYAN | (VGA_BLACK << 4));

        char support_str[40] = "Support: ";
        if (simd_is_supported(SIMD_SSE)) strcat(support_str, "SSE ");
        if (simd_is_supported(SIMD_SSE2)) strcat(support_str, "SSE2 ");
        if (simd_is_supported(SIMD_AVX)) strcat(support_str, "AVX ");
        if (simd_is_supported(SIMD_AVX2)) strcat(support_str, "AVX2");
        vga_puts_at(support_str, 22, 0, VGA_YELLOW | (VGA_BLACK << 4));

        char enabled_str[40] = "Enabled: ";
        if (simd_is_enabled(SIMD_SSE)) strcat(enabled_str, "SSE ");
        if (simd_is_enabled(SIMD_AVX)) strcat(enabled_str, "AVX ");
        if (simd_is_enabled(SIMD_AVX512F)) strcat(enabled_str, "AVX512 ");
        if (simd_is_enabled(SIMD_FMA)) strcat(enabled_str, "FMA");
        vga_puts_at(enabled_str, 23, 0, VGA_GREEN | (VGA_BLACK << 4));

        u32 state_size = simd_get_state_size();
        char state_str[40];
        sprintf(state_str, "State size: %u bytes", state_size);
        vga_puts_at(state_str, 24, 0, VGA_LBLUE | (VGA_BLACK << 4));
    } else {
        vga_puts_at("SIMD optimization not available", 21, 0, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(3000);
    }
}

static void interrupt_mgmt_demo_task(void) {
    vga_puts_at("Interrupt Management:", 20, 40, VGA_WHITE | (VGA_BLACK << 4));

    char support_str[40] = "Support: ";
    if (interrupt_mgmt_is_nmi_supported()) strcat(support_str, "NMI ");
    if (interrupt_mgmt_is_smi_supported()) strcat(support_str, "SMI ");
    if (interrupt_mgmt_is_mce_supported()) strcat(support_str, "MCE ");
    if (interrupt_mgmt_is_thermal_interrupt_supported()) strcat(support_str, "THM");
    vga_puts_at(support_str, 21, 40, VGA_CYAN | (VGA_BLACK << 4));

    u32 nmi_count = interrupt_mgmt_get_nmi_count();
    u32 mce_count = interrupt_mgmt_get_mce_count();

    char counts_str[40];
    sprintf(counts_str, "NMI: %u MCE: %u", nmi_count, mce_count);
    vga_puts_at(counts_str, 22, 40, VGA_YELLOW | (VGA_BLACK << 4));

    if (interrupt_mgmt_is_mce_supported()) {
        u32 mce_banks = interrupt_mgmt_get_mce_bank_count();
        char banks_str[40];
        sprintf(banks_str, "MCE banks: %u", mce_banks);
        vga_puts_at(banks_str, 23, 40, VGA_GREEN | (VGA_BLACK << 4));
    }

    u8 nesting = interrupt_mgmt_get_nesting_level();
    u32 last_vector = interrupt_mgmt_get_last_vector();

    char nest_str[40];
    sprintf(nest_str, "Nest: %u Last: %u", nesting, last_vector);
    vga_puts_at(nest_str, 24, 40, VGA_LGRAY | (VGA_BLACK << 4));

    while (1) {
        timer_delay(4000);
    }
}

static void atomic_demo_task(void) {
    vga_set_cursor(12, 0);
    vga_puts("Atomic Operations:\n");

    if (atomic_is_supported()) {
        u8 smp = atomic_is_smp_system();
        u32 cpu_count = atomic_get_cpu_count();
        u32 cache_line = atomic_get_cache_line_size();

        char system_str[40];
        sprintf(system_str, "System: %s CPUs: %u", smp ? "SMP" : "UP", cpu_count);
        vga_puts_at(system_str, 13, 0, VGA_CYAN | (VGA_BLACK << 4));

        char cache_str[40];
        sprintf(cache_str, "Cache line: %u bytes", cache_line);
        vga_puts_at(cache_str, 14, 0, VGA_YELLOW | (VGA_BLACK << 4));

        /* Test atomic operations */
        static volatile u32 test_counter = 0;
        u32 old_value = atomic_inc_u32(&test_counter);

        char test_str[40];
        sprintf(test_str, "Test counter: %u", old_value);
        vga_puts_at(test_str, 15, 0, VGA_GREEN | (VGA_BLACK << 4));

        /* Test spinlock */
        static atomic_spinlock_t test_lock;
        static u8 lock_initialized = 0;

        if (!lock_initialized) {
            atomic_spinlock_init(&test_lock);
            lock_initialized = 1;
        }

        u8 locked = atomic_spinlock_is_locked(&test_lock);
        char lock_str[40];
        sprintf(lock_str, "Spinlock: %s", locked ? "Locked" : "Free");
        vga_puts_at(lock_str, 16, 0, VGA_LBLUE | (VGA_BLACK << 4));

        vga_puts_at("Features: CAS, XCHG, LOCK", 17, 0, VGA_WHITE | (VGA_BLACK << 4));
    } else {
        vga_puts_at("Atomic operations not supported", 13, 0, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(5000);
    }
}

static void power_mgmt_demo_task(void) {
    vga_puts_at("Power Management:", 20, 0, VGA_WHITE | (VGA_BLACK << 4));

    if (power_mgmt_is_supported()) {
        u8 cstate_support = power_mgmt_is_cstate_supported();
        u8 energy_support = power_mgmt_is_energy_monitoring_supported();

        char support_str[40];
        sprintf(support_str, "Support: %s %s",
                cstate_support ? "C-states" : "",
                energy_support ? "Energy" : "");
        vga_puts_at(support_str, 21, 0, VGA_CYAN | (VGA_BLACK << 4));

        if (cstate_support) {
            u8 num_cstates = power_mgmt_get_num_cstates();
            u8 deepest = power_mgmt_get_deepest_cstate();
            u8 current = power_mgmt_get_current_cstate();

            char cstate_str[40];
            sprintf(cstate_str, "C-states: %u/%u Current: C%u", num_cstates, deepest, current);
            vga_puts_at(cstate_str, 22, 0, VGA_YELLOW | (VGA_BLACK << 4));
        }

        if (energy_support) {
            u32 current_power = power_mgmt_get_current_power();
            u32 tdp = power_mgmt_get_thermal_design_power();

            char power_str[40];
            sprintf(power_str, "Power: %uW TDP: %uW", current_power, tdp);
            vga_puts_at(power_str, 23, 0, VGA_GREEN | (VGA_BLACK << 4));
        }

        u8 pkg_cstates = power_mgmt_are_package_cstates_enabled();
        char pkg_str[40];
        sprintf(pkg_str, "Package C-states: %s", pkg_cstates ? "Enabled" : "Disabled");
        vga_puts_at(pkg_str, 24, 0, VGA_LBLUE | (VGA_BLACK << 4));
    } else {
        vga_puts_at("Power management not supported", 21, 0, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(3000);
    }
}

static void memory_protection_demo_task(void) {
    vga_puts_at("Memory Protection:", 20, 40, VGA_WHITE | (VGA_BLACK << 4));

    if (memory_protection_is_supported()) {
        char support_str[40] = "Support: ";
        if (memory_protection_is_smep_supported()) strcat(support_str, "SMEP ");
        if (memory_protection_is_smap_supported()) strcat(support_str, "SMAP ");
        if (memory_protection_is_pku_supported()) strcat(support_str, "PKU ");
        if (memory_protection_is_cet_supported()) strcat(support_str, "CET");
        vga_puts_at(support_str, 21, 40, VGA_CYAN | (VGA_BLACK << 4));

        char enabled_str[40] = "Enabled: ";
        if (memory_protection_is_smep_enabled()) strcat(enabled_str, "SMEP ");
        if (memory_protection_is_smap_enabled()) strcat(enabled_str, "SMAP ");
        if (memory_protection_is_pku_enabled()) strcat(enabled_str, "PKU ");
        if (memory_protection_is_cet_enabled()) strcat(enabled_str, "CET");
        vga_puts_at(enabled_str, 22, 40, VGA_YELLOW | (VGA_BLACK << 4));

        if (memory_protection_is_cet_enabled()) {
            u8 shadow_stack = memory_protection_is_shadow_stack_enabled();
            u8 ibt = memory_protection_is_indirect_branch_tracking_enabled();

            char cet_str[40];
            sprintf(cet_str, "CET: %s %s",
                    shadow_stack ? "SS" : "",
                    ibt ? "IBT" : "");
            vga_puts_at(cet_str, 23, 40, VGA_GREEN | (VGA_BLACK << 4));
        }

        u32 violations = memory_protection_get_violation_count();
        char viol_str[40];
        sprintf(viol_str, "Violations: %u", violations);
        vga_puts_at(viol_str, 24, 40, VGA_LGRAY | (VGA_BLACK << 4));
    } else {
        vga_puts_at("Memory protection not supported", 21, 40, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(4000);
    }
}

static void virt_ext_demo_task(void) {
    vga_set_cursor(12, 40);
    vga_puts("Virtualization Extensions:\n");

    if (virt_ext_is_supported()) {
        char support_str[40] = "Support: ";
        if (virt_ext_is_vmx_supported()) strcat(support_str, "VMX ");
        if (virt_ext_is_svm_supported()) strcat(support_str, "SVM");
        vga_puts_at(support_str, 13, 40, VGA_CYAN | (VGA_BLACK << 4));

        if (virt_ext_is_vmx_supported()) {
            u8 vmx_enabled = virt_ext_is_vmx_enabled();
            u8 vmx_state = virt_ext_get_vmx_operation_state();

            char vmx_str[40];
            sprintf(vmx_str, "VMX: %s State: %u",
                    vmx_enabled ? "Enabled" : "Disabled", vmx_state);
            vga_puts_at(vmx_str, 14, 40, VGA_YELLOW | (VGA_BLACK << 4));

            char features_str[40] = "Features: ";
            if (virt_ext_is_ept_supported()) strcat(features_str, "EPT ");
            if (virt_ext_is_vpid_supported()) strcat(features_str, "VPID ");
            if (virt_ext_is_unrestricted_guest_supported()) strcat(features_str, "UG");
            vga_puts_at(features_str, 15, 40, VGA_GREEN | (VGA_BLACK << 4));

            u32 exits = virt_ext_get_vm_exit_count();
            u32 entries = virt_ext_get_vm_entry_count();

            char stats_str[40];
            sprintf(stats_str, "Exits: %u Entries: %u", exits, entries);
            vga_puts_at(stats_str, 16, 40, VGA_LBLUE | (VGA_BLACK << 4));

            char advanced_str[40] = "Advanced: ";
            if (virt_ext_is_vmfunc_supported()) strcat(advanced_str, "VMFUNC ");
            if (virt_ext_is_nested_virtualization_supported()) strcat(advanced_str, "NESTED");
            vga_puts_at(advanced_str, 17, 40, VGA_WHITE | (VGA_BLACK << 4));
        }
    } else {
        vga_puts_at("Virtualization not supported", 13, 40, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(5000);
    }
}

static void debug_hw_demo_task(void) {
    vga_puts_at("Hardware Debugging:", 20, 0, VGA_WHITE | (VGA_BLACK << 4));

    if (debug_hw_is_supported()) {
        u8 num_bp = debug_hw_get_num_breakpoints();

        char bp_str[40];
        sprintf(bp_str, "Breakpoints: %u available", num_bp);
        vga_puts_at(bp_str, 21, 0, VGA_CYAN | (VGA_BLACK << 4));

        u8 single_step = debug_hw_is_single_step_enabled();
        u8 branch_trace = debug_hw_is_branch_trace_enabled();

        char features_str[40];
        sprintf(features_str, "Features: %s %s",
                single_step ? "SingleStep" : "",
                branch_trace ? "BranchTrace" : "");
        vga_puts_at(features_str, 22, 0, VGA_YELLOW | (VGA_BLACK << 4));

        u32 debug_exceptions = debug_hw_get_debug_exception_count();
        u32 single_steps = debug_hw_get_single_step_count();

        char stats_str[40];
        sprintf(stats_str, "Exceptions: %u Steps: %u", debug_exceptions, single_steps);
        vga_puts_at(stats_str, 23, 0, VGA_GREEN | (VGA_BLACK << 4));

        u64 dr6 = debug_hw_get_dr6_status();
        u64 dr7 = debug_hw_get_dr7_control();

        char regs_str[40];
        sprintf(regs_str, "DR6: %08X DR7: %08X", (u32)dr6, (u32)dr7);
        vga_puts_at(regs_str, 24, 0, VGA_LBLUE | (VGA_BLACK << 4));
    } else {
        vga_puts_at("Hardware debugging not supported", 21, 0, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(3000);
    }
}

static void pmu_ext_demo_task(void) {
    vga_puts_at("PMU Extensions:", 20, 40, VGA_WHITE | (VGA_BLACK << 4));

    if (pmu_ext_is_supported()) {
        u8 version = pmu_ext_get_version();
        u8 gp_counters = pmu_ext_get_num_gp_counters();
        u8 fixed_counters = pmu_ext_get_num_fixed_counters();

        char version_str[40];
        sprintf(version_str, "Version: %u GP: %u Fixed: %u", version, gp_counters, fixed_counters);
        vga_puts_at(version_str, 21, 40, VGA_CYAN | (VGA_BLACK << 4));

        char support_str[40] = "Support: ";
        if (pmu_ext_is_pebs_supported()) strcat(support_str, "PEBS ");
        if (pmu_ext_is_lbr_supported()) strcat(support_str, "LBR");
        vga_puts_at(support_str, 22, 40, VGA_YELLOW | (VGA_BLACK << 4));

        u32 pmi_count = pmu_ext_get_pmi_count();
        u32 overflow_count = pmu_ext_get_overflow_count();

        char stats_str[40];
        sprintf(stats_str, "PMI: %u Overflows: %u", pmi_count, overflow_count);
        vga_puts_at(stats_str, 23, 40, VGA_GREEN | (VGA_BLACK << 4));

        u64 global_status = pmu_ext_get_global_status();
        char status_str[40];
        sprintf(status_str, "Global Status: %08X", (u32)global_status);
        vga_puts_at(status_str, 24, 40, VGA_LGRAY | (VGA_BLACK << 4));
    } else {
        vga_puts_at("PMU extensions not supported", 21, 40, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(4000);
    }
}

static void trace_hw_demo_task(void) {
    vga_set_cursor(12, 0);
    vga_puts("Hardware Tracing:\n");

    if (trace_hw_is_supported()) {
        char support_str[40] = "Support: ";
        if (trace_hw_is_intel_pt_supported()) strcat(support_str, "Intel-PT ");
        if (trace_hw_is_bts_supported()) strcat(support_str, "BTS");
        vga_puts_at(support_str, 13, 0, VGA_CYAN | (VGA_BLACK << 4));

        if (trace_hw_is_intel_pt_supported()) {
            u8 pt_enabled = trace_hw_is_intel_pt_enabled();
            u8 num_ranges = trace_hw_get_num_address_ranges();

            char pt_str[40];
            sprintf(pt_str, "Intel PT: %s Ranges: %u",
                    pt_enabled ? "Enabled" : "Disabled", num_ranges);
            vga_puts_at(pt_str, 14, 0, VGA_YELLOW | (VGA_BLACK << 4));

            char features_str[40] = "Features: ";
            if (trace_hw_is_cycle_accurate_supported()) strcat(features_str, "CYC ");
            if (trace_hw_is_timing_packets_supported()) strcat(features_str, "TSC ");
            if (trace_hw_is_power_event_trace_supported()) strcat(features_str, "PWR");
            vga_puts_at(features_str, 15, 0, VGA_GREEN | (VGA_BLACK << 4));
        }

        if (trace_hw_is_bts_supported()) {
            u8 bts_enabled = trace_hw_is_bts_enabled();
            u32 bts_records = trace_hw_get_num_bts_records();

            char bts_str[40];
            sprintf(bts_str, "BTS: %s Records: %u",
                    bts_enabled ? "Enabled" : "Disabled", bts_records);
            vga_puts_at(bts_str, 16, 0, VGA_LBLUE | (VGA_BLACK << 4));
        }

        u32 packets = trace_hw_get_packets_generated();
        u32 overflows = trace_hw_get_buffer_overflows();
        u32 errors = trace_hw_get_trace_errors();

        char stats_str[40];
        sprintf(stats_str, "Packets: %u Overflows: %u Errors: %u", packets, overflows, errors);
        vga_puts_at(stats_str, 17, 0, VGA_WHITE | (VGA_BLACK << 4));
    } else {
        vga_puts_at("Hardware tracing not supported", 13, 0, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(5000);
    }
}

static void exception_mgmt_demo_task(void) {
    vga_puts_at("Exception Management:", 20, 0, VGA_WHITE | (VGA_BLACK << 4));

    if (exception_mgmt_is_supported()) {
        u32 total = exception_mgmt_get_total_exceptions();
        u32 recovered = exception_mgmt_get_recovered_exceptions();
        u32 unhandled = exception_mgmt_get_unhandled_exceptions();

        char stats_str[40];
        sprintf(stats_str, "Total: %u Recovered: %u", total, recovered);
        vga_puts_at(stats_str, 21, 0, VGA_CYAN | (VGA_BLACK << 4));

        char unhandled_str[40];
        sprintf(unhandled_str, "Unhandled: %u", unhandled);
        vga_puts_at(unhandled_str, 22, 0, VGA_YELLOW | (VGA_BLACK << 4));

        char features_str[40] = "Features: ";
        if (exception_mgmt_is_recovery_enabled()) strcat(features_str, "Recovery ");
        if (exception_mgmt_is_logging_enabled()) strcat(features_str, "Logging ");
        if (exception_mgmt_is_fault_injection_enabled()) strcat(features_str, "Injection");
        vga_puts_at(features_str, 23, 0, VGA_GREEN | (VGA_BLACK << 4));

        u64 last_df = exception_mgmt_get_last_double_fault_time();
        u32 nested = exception_mgmt_get_nested_exception_count();

        char special_str[40];
        sprintf(special_str, "DoubleFault: %u Nested: %u", (u32)last_df, nested);
        vga_puts_at(special_str, 24, 0, VGA_LBLUE | (VGA_BLACK << 4));
    } else {
        vga_puts_at("Exception management not supported", 21, 0, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(3000);
    }
}

static void memory_adv_demo_task(void) {
    vga_puts_at("Advanced Memory:", 20, 40, VGA_WHITE | (VGA_BLACK << 4));

    if (memory_adv_is_supported()) {
        char support_str[40] = "Support: ";
        if (memory_adv_is_pcid_supported()) strcat(support_str, "PCID ");
        if (memory_adv_is_invpcid_supported()) strcat(support_str, "INVPCID ");
        if (memory_adv_is_pat_supported()) strcat(support_str, "PAT ");
        if (memory_adv_is_mtrr_supported()) strcat(support_str, "MTRR");
        vga_puts_at(support_str, 21, 40, VGA_CYAN | (VGA_BLACK << 4));

        if (memory_adv_is_pcid_enabled()) {
            u16 current_pcid = memory_adv_get_current_pcid();

            char pcid_str[40];
            sprintf(pcid_str, "Current PCID: %u", current_pcid);
            vga_puts_at(pcid_str, 22, 40, VGA_YELLOW | (VGA_BLACK << 4));
        }

        u32 tlb_flushes = memory_adv_get_total_tlb_flushes();
        u32 pcid_switches = memory_adv_get_pcid_switches();
        u32 invpcid_calls = memory_adv_get_invpcid_calls();

        char stats_str[40];
        sprintf(stats_str, "TLB: %u PCID: %u INV: %u", tlb_flushes, pcid_switches, invpcid_calls);
        vga_puts_at(stats_str, 23, 40, VGA_GREEN | (VGA_BLACK << 4));

        u8 mtrr_entries = memory_adv_get_num_mtrr_entries();
        u64 pat_value = memory_adv_get_pat_value();

        char config_str[40];
        sprintf(config_str, "MTRR: %u PAT: %08X", mtrr_entries, (u32)pat_value);
        vga_puts_at(config_str, 24, 40, VGA_LGRAY | (VGA_BLACK << 4));
    } else {
        vga_puts_at("Advanced memory not supported", 21, 40, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(4000);
    }
}

static void security_ext_demo_task(void) {
    vga_set_cursor(12, 40);
    vga_puts("Security Extensions:\n");

    if (security_ext_is_supported()) {
        char support_str[40] = "Support: ";
        if (security_ext_is_mpx_supported()) strcat(support_str, "MPX ");
        if (security_ext_is_cet_supported()) strcat(support_str, "CET");
        vga_puts_at(support_str, 13, 40, VGA_CYAN | (VGA_BLACK << 4));

        char enabled_str[40] = "Enabled: ";
        if (security_ext_is_mpx_enabled()) strcat(enabled_str, "MPX ");
        if (security_ext_is_cet_enabled()) strcat(enabled_str, "CET ");
        if (security_ext_is_nx_enabled()) strcat(enabled_str, "NX");
        vga_puts_at(enabled_str, 14, 40, VGA_YELLOW | (VGA_BLACK << 4));

        if (security_ext_is_cet_enabled()) {
            char cet_str[40] = "CET: ";
            if (security_ext_is_shadow_stack_enabled()) strcat(cet_str, "SS ");
            if (security_ext_is_ibt_enabled()) strcat(cet_str, "IBT");
            vga_puts_at(cet_str, 15, 40, VGA_GREEN | (VGA_BLACK << 4));
        }

        u32 total_violations = security_ext_get_violation_count();
        u32 mpx_violations = security_ext_get_mpx_violations();
        u32 cet_violations = security_ext_get_cet_violations();

        char viol_str[40];
        sprintf(viol_str, "Violations: %u MPX: %u CET: %u", total_violations, mpx_violations, cet_violations);
        vga_puts_at(viol_str, 16, 40, VGA_LBLUE | (VGA_BLACK << 4));

        if (security_ext_is_shadow_stack_enabled()) {
            u64 ssp = security_ext_get_shadow_stack_pointer();
            char ssp_str[40];
            sprintf(ssp_str, "SSP: %08X", (u32)ssp);
            vga_puts_at(ssp_str, 17, 40, VGA_WHITE | (VGA_BLACK << 4));
        }
    } else {
        vga_puts_at("Security extensions not supported", 13, 40, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(5000);
    }
}

static void freq_scaling_demo_task(void) {
    vga_puts_at("Frequency Scaling:", 20, 0, VGA_WHITE | (VGA_BLACK << 4));

    if (freq_scaling_is_supported()) {
        char support_str[40] = "Support: ";
        if (freq_scaling_is_speedstep_supported()) strcat(support_str, "SpeedStep ");
        if (freq_scaling_is_turbo_supported()) strcat(support_str, "Turbo");
        vga_puts_at(support_str, 21, 0, VGA_CYAN | (VGA_BLACK << 4));

        u16 current_freq = freq_scaling_get_current_frequency();
        u16 max_freq = freq_scaling_get_max_frequency();
        u16 base_freq = freq_scaling_get_base_frequency();

        char freq_str[40];
        sprintf(freq_str, "Freq: %uMHz Max: %uMHz Base: %uMHz", current_freq, max_freq, base_freq);
        vga_puts_at(freq_str, 22, 0, VGA_YELLOW | (VGA_BLACK << 4));

        u8 num_pstates = freq_scaling_get_num_pstates();
        u8 current_pstate = freq_scaling_get_current_pstate();
        u8 turbo_enabled = freq_scaling_is_turbo_enabled();

        char state_str[40];
        sprintf(state_str, "P-states: %u Current: %u Turbo: %s",
                num_pstates, current_pstate, turbo_enabled ? "On" : "Off");
        vga_puts_at(state_str, 23, 0, VGA_GREEN | (VGA_BLACK << 4));

        u32 transitions = freq_scaling_get_transition_count();
        u8 epb = freq_scaling_get_energy_perf_bias();

        char stats_str[40];
        sprintf(stats_str, "Transitions: %u EPB: %u", transitions, epb);
        vga_puts_at(stats_str, 24, 0, VGA_LBLUE | (VGA_BLACK << 4));
    } else {
        vga_puts_at("Frequency scaling not supported", 21, 0, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(3000);
    }
}

static void system_ctrl_demo_task(void) {
    vga_puts_at("System Control:", 20, 40, VGA_WHITE | (VGA_BLACK << 4));

    if (system_ctrl_is_supported()) {
        char support_str[40] = "Support: ";
        if (system_ctrl_is_smm_supported()) strcat(support_str, "SMM ");
        if (system_ctrl_is_acpi_supported()) strcat(support_str, "ACPI");
        vga_puts_at(support_str, 21, 40, VGA_CYAN | (VGA_BLACK << 4));

        if (system_ctrl_is_smm_supported()) {
            u32 smi_count = system_ctrl_get_smi_count();
            u8 smm_enabled = system_ctrl_is_smm_enabled();

            char smm_str[40];
            sprintf(smm_str, "SMM: %s SMI Count: %u",
                    smm_enabled ? "Enabled" : "Disabled", smi_count);
            vga_puts_at(smm_str, 22, 40, VGA_YELLOW | (VGA_BLACK << 4));
        }

        if (system_ctrl_is_acpi_supported()) {
            u8 acpi_state = system_ctrl_get_acpi_state();
            char acpi_str[40] = "ACPI: S";
            char state_char = '0' + acpi_state;
            strncat(acpi_str, &state_char, 1);
            strcat(acpi_str, " ");
            if (system_ctrl_is_s3_supported()) strcat(acpi_str, "S3 ");
            if (system_ctrl_is_s4_supported()) strcat(acpi_str, "S4 ");
            if (system_ctrl_is_s5_supported()) strcat(acpi_str, "S5");
            vga_puts_at(acpi_str, 23, 40, VGA_GREEN | (VGA_BLACK << 4));
        }

        u64 uptime = system_ctrl_get_uptime();
        u32 resets = system_ctrl_get_reset_count();

        char stats_str[40];
        sprintf(stats_str, "Uptime: %us Resets: %u", (u32)uptime, resets);
        vga_puts_at(stats_str, 24, 40, VGA_LGRAY | (VGA_BLACK << 4));
    } else {
        vga_puts_at("System control not supported", 21, 40, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(4000);
    }
}

static void cpu_ext_demo_task(void) {
    vga_set_cursor(12, 0);
    vga_puts("CPU Extensions:\n");

    if (cpu_ext_is_supported()) {
        char support_str[40] = "Support: ";
        if (cpu_ext_is_avx512_supported()) strcat(support_str, "AVX-512 ");
        if (cpu_ext_is_amx_supported()) strcat(support_str, "AMX");
        vga_puts_at(support_str, 13, 0, VGA_CYAN | (VGA_BLACK << 4));

        char enabled_str[40] = "Enabled: ";
        if (cpu_ext_is_avx512_enabled()) strcat(enabled_str, "AVX-512 ");
        if (cpu_ext_is_amx_enabled()) strcat(enabled_str, "AMX");
        vga_puts_at(enabled_str, 14, 0, VGA_YELLOW | (VGA_BLACK << 4));

        if (cpu_ext_is_avx512_supported()) {
            u32 avx512_features = cpu_ext_get_avx512_features();
            char avx512_str[40];
            sprintf(avx512_str, "AVX-512 Features: %08X", avx512_features);
            vga_puts_at(avx512_str, 15, 0, VGA_GREEN | (VGA_BLACK << 4));
        }

        if (cpu_ext_is_amx_supported()) {
            u8 num_tiles = cpu_ext_get_num_tiles();
            u16 tile_rows = cpu_ext_get_tile_max_rows();

            char amx_str[40];
            sprintf(amx_str, "AMX: %u tiles, %u max rows", num_tiles, tile_rows);
            vga_puts_at(amx_str, 16, 0, VGA_LBLUE | (VGA_BLACK << 4));
        }

        u32 xsave_size = cpu_ext_get_xsave_area_size();
        u64 xcr0 = cpu_ext_get_xcr0_value();

        char xsave_str[40];
        sprintf(xsave_str, "XSAVE: %u bytes XCR0: %08X", xsave_size, (u32)xcr0);
        vga_puts_at(xsave_str, 17, 0, VGA_WHITE | (VGA_BLACK << 4));
    } else {
        vga_puts_at("CPU extensions not supported", 13, 0, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(5000);
    }
}

static void microarch_demo_task(void) {
    vga_puts_at("Microarchitecture:", 20, 0, VGA_WHITE | (VGA_BLACK << 4));

    if (microarch_is_supported()) {
        char support_str[40] = "Support: ";
        if (microarch_is_spec_ctrl_supported()) strcat(support_str, "SpecCtrl ");
        if (microarch_is_ibrs_supported()) strcat(support_str, "IBRS ");
        if (microarch_is_stibp_supported()) strcat(support_str, "STIBP ");
        if (microarch_is_ssbd_supported()) strcat(support_str, "SSBD");
        vga_puts_at(support_str, 21, 0, VGA_CYAN | (VGA_BLACK << 4));

        char enabled_str[40] = "Enabled: ";
        if (microarch_is_ibrs_enabled()) strcat(enabled_str, "IBRS ");
        if (microarch_is_stibp_enabled()) strcat(enabled_str, "STIBP ");
        if (microarch_is_ssbd_enabled()) strcat(enabled_str, "SSBD");
        vga_puts_at(enabled_str, 22, 0, VGA_YELLOW | (VGA_BLACK << 4));

        u32 mitigations = microarch_get_speculation_mitigations();
        u32 branch_flushes = microarch_get_branch_flushes();

        char stats_str[40];
        sprintf(stats_str, "Mitigations: %u Flushes: %u", mitigations, branch_flushes);
        vga_puts_at(stats_str, 23, 0, VGA_GREEN | (VGA_BLACK << 4));

        const branch_stats_t* branch_stats = microarch_get_branch_stats();
        char branch_str[40];
        sprintf(branch_str, "Branches: %u Misses: %u Rate: %u%%",
                (u32)branch_stats->total_branches, (u32)branch_stats->branch_misses,
                branch_stats->misprediction_rate);
        vga_puts_at(branch_str, 24, 0, VGA_LBLUE | (VGA_BLACK << 4));
    } else {
        vga_puts_at("Microarchitecture not supported", 21, 0, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(3000);
    }
}

static void apic_ext_demo_task(void) {
    vga_puts_at("APIC Extensions:", 20, 40, VGA_WHITE | (VGA_BLACK << 4));

    if (apic_ext_is_supported()) {
        u32 apic_id = apic_ext_get_id();
        u32 version = apic_ext_get_version();
        u8 max_lvt = apic_ext_get_max_lvt_entries();

        char info_str[40];
        sprintf(info_str, "ID: %u Ver: %u LVT: %u", apic_id, version, max_lvt);
        vga_puts_at(info_str, 21, 40, VGA_CYAN | (VGA_BLACK << 4));

        char support_str[40] = "Support: ";
        if (apic_ext_is_x2apic_supported()) strcat(support_str, "x2APIC ");
        if (apic_ext_is_x2apic_enabled()) strcat(support_str, "(Enabled)");
        vga_puts_at(support_str, 22, 40, VGA_YELLOW | (VGA_BLACK << 4));

        u32 ipi_count = apic_ext_get_ipi_count();
        u32 nmi_count = apic_ext_get_nmi_count();
        u32 broadcast_count = apic_ext_get_broadcast_count();

        char ipi_str[40];
        sprintf(ipi_str, "IPI: %u NMI: %u Broadcast: %u", ipi_count, nmi_count, broadcast_count);
        vga_puts_at(ipi_str, 23, 40, VGA_GREEN | (VGA_BLACK << 4));

        u32 error_count = apic_ext_get_error_count();
        u32 last_error = apic_ext_get_last_error();

        char error_str[40];
        sprintf(error_str, "Errors: %u Last: %08X", error_count, last_error);
        vga_puts_at(error_str, 24, 40, VGA_LGRAY | (VGA_BLACK << 4));
    } else {
        vga_puts_at("APIC extensions not supported", 21, 40, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(4000);
    }
}

static void timing_sync_demo_task(void) {
    vga_set_cursor(12, 40);
    vga_puts("Timing & Sync:\n");

    if (timing_sync_is_supported()) {
        char support_str[40] = "Support: ";
        if (timing_sync_is_tsc_supported()) strcat(support_str, "TSC ");
        if (timing_sync_is_hpet_supported()) strcat(support_str, "HPET ");
        if (timing_sync_is_rdtscp_supported()) strcat(support_str, "RDTSCP");
        vga_puts_at(support_str, 13, 40, VGA_CYAN | (VGA_BLACK << 4));

        if (timing_sync_is_tsc_supported()) {
            u64 tsc_freq = timing_sync_get_tsc_frequency();
            u8 invariant = timing_sync_is_tsc_invariant();

            char tsc_str[40];
            sprintf(tsc_str, "TSC: %uMHz %s", (u32)(tsc_freq / 1000000),
                    invariant ? "Invariant" : "Variable");
            vga_puts_at(tsc_str, 14, 40, VGA_YELLOW | (VGA_BLACK << 4));
        }

        if (timing_sync_is_hpet_supported()) {
            u64 hpet_freq = timing_sync_get_hpet_frequency();
            u8 num_timers = timing_sync_get_hpet_num_timers();
            u8 counter_size = timing_sync_get_hpet_counter_size();

            char hpet_str[40];
            sprintf(hpet_str, "HPET: %uMHz %ut %ub", (u32)(hpet_freq / 1000000),
                    num_timers, counter_size);
            vga_puts_at(hpet_str, 15, 40, VGA_GREEN | (VGA_BLACK << 4));
        }

        u32 sync_ops = timing_sync_get_sync_operations();
        u32 measurements = timing_sync_get_timing_measurements();

        char stats_str[40];
        sprintf(stats_str, "Sync: %u Measure: %u", sync_ops, measurements);
        vga_puts_at(stats_str, 16, 40, VGA_LBLUE | (VGA_BLACK << 4));

        u64 max_latency = timing_sync_get_max_sync_latency();
        u64 min_latency = timing_sync_get_min_sync_latency();

        char latency_str[40];
        sprintf(latency_str, "Latency: %u-%u cycles", (u32)min_latency, (u32)max_latency);
        vga_puts_at(latency_str, 17, 40, VGA_WHITE | (VGA_BLACK << 4));
    } else {
        vga_puts_at("Timing & sync not supported", 13, 40, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(5000);
    }
}

static void cache_coherency_demo_task(void) {
    vga_puts_at("Cache Coherency:", 12, 0, VGA_WHITE | (VGA_BLACK << 4));

    if (cache_coherency_is_supported()) {
        char support_str[40] = "Support: ";
        if (cache_coherency_is_mesi_supported()) strcat(support_str, "MESI ");
        if (cache_coherency_is_moesi_supported()) strcat(support_str, "MOESI ");
        if (cache_coherency_is_monitoring_supported()) strcat(support_str, "Monitor");
        vga_puts_at(support_str, 13, 0, VGA_CYAN | (VGA_BLACK << 4));

        u8 num_levels = cache_coherency_get_num_levels();
        const cache_level_info_t* l1 = cache_coherency_get_level_info(0);

        if (l1) {
            char cache_str[40];
            sprintf(cache_str, "L1: %uKB %uB/line %u-way", l1->size_kb, l1->line_size, l1->associativity);
            vga_puts_at(cache_str, 14, 0, VGA_YELLOW | (VGA_BLACK << 4));
        }

        u32 operations = cache_coherency_get_total_operations();
        u32 coherency_trans = cache_coherency_get_coherency_transactions();
        u32 snoop_hits = cache_coherency_get_snoop_hits();

        char stats_str[40];
        sprintf(stats_str, "Ops: %u Coherency: %u Snoop: %u", operations, coherency_trans, snoop_hits);
        vga_puts_at(stats_str, 15, 0, VGA_GREEN | (VGA_BLACK << 4));

        u32 flushes = cache_coherency_get_cache_flushes();
        u32 invalidations = cache_coherency_get_cache_invalidations();

        char flush_str[40];
        sprintf(flush_str, "Flushes: %u Invalidations: %u", flushes, invalidations);
        vga_puts_at(flush_str, 16, 0, VGA_LBLUE | (VGA_BLACK << 4));
    } else {
        vga_puts_at("Cache coherency not supported", 13, 0, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(3000);
    }
}

static void memory_ordering_demo_task(void) {
    vga_puts_at("Memory Ordering:", 12, 40, VGA_WHITE | (VGA_BLACK << 4));

    if (memory_ordering_is_supported()) {
        u8 strong_ordering = memory_ordering_is_strong_ordering();
        u8 memory_model = memory_ordering_get_memory_model();

        char model_str[40];
        sprintf(model_str, "Model: %s %s", memory_ordering_get_memory_model_name(memory_model),
                strong_ordering ? "(Strong)" : "(Weak)");
        vga_puts_at(model_str, 13, 40, VGA_CYAN | (VGA_BLACK << 4));

        u32 total_fences = memory_ordering_get_total_fences();
        u32 load_fences = memory_ordering_get_load_fences();
        u32 store_fences = memory_ordering_get_store_fences();
        u32 full_fences = memory_ordering_get_full_fences();

        char fence_str[40];
        sprintf(fence_str, "Fences: %u L:%u S:%u F:%u", total_fences, load_fences, store_fences, full_fences);
        vga_puts_at(fence_str, 14, 40, VGA_YELLOW | (VGA_BLACK << 4));

        u64 max_latency = memory_ordering_get_max_fence_latency();
        u64 min_latency = memory_ordering_get_min_fence_latency();
        u32 violations = memory_ordering_get_ordering_violations();

        char latency_str[40];
        sprintf(latency_str, "Latency: %u-%u Violations: %u", (u32)min_latency, (u32)max_latency, violations);
        vga_puts_at(latency_str, 15, 40, VGA_GREEN | (VGA_BLACK << 4));

        u32 serialize_fences = memory_ordering_get_serialize_fences();

        char serialize_str[40];
        sprintf(serialize_str, "Serialize: %u", serialize_fences);
        vga_puts_at(serialize_str, 16, 40, VGA_LBLUE | (VGA_BLACK << 4));
    } else {
        vga_puts_at("Memory ordering not supported", 13, 40, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(4000);
    }
}

static void power_states_demo_task(void) {
    vga_set_cursor(18, 0);
    vga_puts("Power States:\n");

    if (power_states_is_supported()) {
        char support_str[40] = "Support: ";
        if (power_states_is_cstates_supported()) strcat(support_str, "C-states ");
        if (power_states_is_pstates_supported()) strcat(support_str, "P-states");
        vga_puts_at(support_str, 19, 0, VGA_CYAN | (VGA_BLACK << 4));

        if (power_states_is_cstates_supported()) {
            u8 current_cstate = power_states_get_current_cstate();
            u8 max_cstate = power_states_get_max_cstate();
            u8 num_cstates = power_states_get_num_cstates();

            char cstate_str[40];
            sprintf(cstate_str, "C-states: %u Current: C%u Max: C%u", num_cstates, current_cstate, max_cstate);
            vga_puts_at(cstate_str, 20, 0, VGA_YELLOW | (VGA_BLACK << 4));
        }

        if (power_states_is_pstates_supported()) {
            u8 current_pstate = power_states_get_current_pstate();
            u8 max_pstate = power_states_get_max_pstate();
            u8 num_pstates = power_states_get_num_pstates();

            char pstate_str[40];
            sprintf(pstate_str, "P-states: %u Current: P%u Max: P%u", num_pstates, current_pstate, max_pstate);
            vga_puts_at(pstate_str, 21, 0, VGA_GREEN | (VGA_BLACK << 4));
        }

        u64 transitions = power_states_get_total_transitions();
        u64 cstate_residency = power_states_get_total_cstate_residency();

        char stats_str[40];
        sprintf(stats_str, "Transitions: %u Residency: %u", (u32)transitions, (u32)cstate_residency);
        vga_puts_at(stats_str, 22, 0, VGA_LBLUE | (VGA_BLACK << 4));

        u16 temperature = power_states_get_current_temperature();
        u8 thermal_throttling = power_states_is_thermal_throttling();

        char thermal_str[40];
        sprintf(thermal_str, "Temp: %uC Throttling: %s", temperature, thermal_throttling ? "Yes" : "No");
        vga_puts_at(thermal_str, 23, 0, VGA_WHITE | (VGA_BLACK << 4));
    } else {
        vga_puts_at("Power states not supported", 19, 0, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(5000);
    }
}

static void cpu_topology_demo_task(void) {
    vga_puts_at("CPU Topology:", 18, 40, VGA_WHITE | (VGA_BLACK << 4));

    if (cpu_topology_is_supported()) {
        char support_str[40] = "Support: ";
        if (cpu_topology_is_x2apic_topology()) strcat(support_str, "x2APIC ");
        if (cpu_topology_is_hybrid_topology()) strcat(support_str, "Hybrid");
        vga_puts_at(support_str, 19, 40, VGA_CYAN | (VGA_BLACK << 4));

        u32 packages = cpu_topology_get_num_packages();
        u32 cores = cpu_topology_get_num_cores();
        u32 threads = cpu_topology_get_num_threads();

        char topo_str[40];
        sprintf(topo_str, "Pkg: %u Cores: %u Threads: %u", packages, cores, threads);
        vga_puts_at(topo_str, 20, 40, VGA_YELLOW | (VGA_BLACK << 4));

        u32 threads_per_core = cpu_topology_get_threads_per_core();
        u32 cores_per_package = cpu_topology_get_cores_per_package();

        char ratio_str[40];
        sprintf(ratio_str, "T/C: %u C/P: %u", threads_per_core, cores_per_package);
        vga_puts_at(ratio_str, 21, 40, VGA_GREEN | (VGA_BLACK << 4));

        u32 bsp_apic_id = cpu_topology_get_bsp_apic_id();
        u8 numa_nodes = cpu_topology_get_numa_nodes();
        u32 cache_line_size = cpu_topology_get_cache_line_size();

        char info_str[40];
        sprintf(info_str, "BSP: %u NUMA: %u CL: %uB", bsp_apic_id, numa_nodes, cache_line_size);
        vga_puts_at(info_str, 22, 40, VGA_LBLUE | (VGA_BLACK << 4));

        u8 num_levels = cpu_topology_get_num_levels();

        char level_str[40];
        sprintf(level_str, "Topology Levels: %u", num_levels);
        vga_puts_at(level_str, 23, 40, VGA_WHITE | (VGA_BLACK << 4));
    } else {
        vga_puts_at("CPU topology not supported", 19, 40, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(3000);
    }
}

static void interrupt_routing_demo_task(void) {
    vga_set_cursor(12, 60);
    vga_puts("Interrupt Routing:\n");

    if (interrupt_routing_is_supported()) {
        char support_str[20] = "Support: ";
        if (interrupt_routing_is_ioapic_supported()) strcat(support_str, "IOAPIC ");
        if (interrupt_routing_is_msi_supported()) strcat(support_str, "MSI ");
        if (interrupt_routing_is_msix_supported()) strcat(support_str, "MSI-X");
        vga_puts_at(support_str, 13, 60, VGA_CYAN | (VGA_BLACK << 4));

        if (interrupt_routing_is_ioapic_supported()) {
            u8 num_ioapics = interrupt_routing_get_num_ioapics();
            const ioapic_info_t* ioapic = interrupt_routing_get_ioapic_info(0);

            if (ioapic) {
                char ioapic_str[20];
                sprintf(ioapic_str, "IOAPIC: %u ID:%u V:%u", num_ioapics, ioapic->id, ioapic->version);
                vga_puts_at(ioapic_str, 14, 60, VGA_YELLOW | (VGA_BLACK << 4));

                char entries_str[20];
                sprintf(entries_str, "Entries: %u", ioapic->max_redirection_entries);
                vga_puts_at(entries_str, 15, 60, VGA_GREEN | (VGA_BLACK << 4));
            }
        }

        u32 total_routed = interrupt_routing_get_total_interrupts_routed();
        u32 msi_interrupts = interrupt_routing_get_msi_interrupts();
        u32 ioapic_interrupts = interrupt_routing_get_ioapic_interrupts();

        char stats_str[20];
        sprintf(stats_str, "Routed: %u", total_routed);
        vga_puts_at(stats_str, 16, 60, VGA_LBLUE | (VGA_BLACK << 4));

        char int_str[20];
        sprintf(int_str, "MSI: %u IOAPIC: %u", msi_interrupts, ioapic_interrupts);
        vga_puts_at(int_str, 17, 60, VGA_WHITE | (VGA_BLACK << 4));
    } else {
        vga_puts_at("Interrupt routing not supported", 13, 60, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(4000);
    }
}

static void cpu_errata_demo_task(void) {
    vga_set_cursor(18, 60);
    vga_puts("CPU Errata:\n");

    if (cpu_errata_is_supported()) {
        char vendor_str[20] = "Vendor: ";
        if (cpu_errata_is_intel()) strcat(vendor_str, "Intel");
        else if (cpu_errata_is_amd()) strcat(vendor_str, "AMD");
        else strcat(vendor_str, "Unknown");
        vga_puts_at(vendor_str, 19, 60, VGA_CYAN | (VGA_BLACK << 4));

        u32 cpu_sig = cpu_errata_get_cpu_signature();
        u32 ucode_rev = cpu_errata_get_microcode_revision();

        char sig_str[20];
        sprintf(sig_str, "Sig: %08X", cpu_sig);
        vga_puts_at(sig_str, 20, 60, VGA_YELLOW | (VGA_BLACK << 4));

        char ucode_str[20];
        sprintf(ucode_str, "uCode: %08X", ucode_rev);
        vga_puts_at(ucode_str, 21, 60, VGA_GREEN | (VGA_BLACK << 4));

        u32 num_errata = cpu_errata_get_num_errata_found();
        u8 has_critical = cpu_errata_has_critical_errata();

        char errata_str[20];
        sprintf(errata_str, "Errata: %u %s", num_errata, has_critical ? "CRIT" : "OK");
        vga_puts_at(errata_str, 22, 60, has_critical ? VGA_RED | (VGA_BLACK << 4) : VGA_LBLUE | (VGA_BLACK << 4));

        u32 workarounds = cpu_errata_get_workarounds_applied();
        u8 needs_update = cpu_errata_needs_microcode_update();

        char work_str[20];
        sprintf(work_str, "Work: %u %s", workarounds, needs_update ? "UPD" : "OK");
        vga_puts_at(work_str, 23, 60, VGA_WHITE | (VGA_BLACK << 4));
    } else {
        vga_puts_at("CPU errata not supported", 19, 60, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(5000);
    }
}

static void virtualization_demo_task(void) {
    vga_set_cursor(12, 0);
    vga_puts("Virtualization:\n");

    if (virtualization_is_supported()) {
        char support_str[40] = "Support: ";
        if (virtualization_is_vmx_supported()) strcat(support_str, "VMX ");
        if (virtualization_is_svm_supported()) strcat(support_str, "SVM");
        vga_puts_at(support_str, 13, 0, VGA_CYAN | (VGA_BLACK << 4));

        char enabled_str[40] = "Enabled: ";
        if (virtualization_is_vmx_enabled()) strcat(enabled_str, "VMX ");
        if (virtualization_is_svm_enabled()) strcat(enabled_str, "SVM");
        vga_puts_at(enabled_str, 14, 0, VGA_YELLOW | (VGA_BLACK << 4));

        if (virtualization_is_vmx_supported()) {
            u32 vmx_caps = virtualization_get_vmx_capabilities();
            char caps_str[40] = "VMX: ";
            if (virtualization_has_ept()) strcat(caps_str, "EPT ");
            if (virtualization_has_vpid()) strcat(caps_str, "VPID ");
            if (virtualization_has_unrestricted_guest()) strcat(caps_str, "UG");
            vga_puts_at(caps_str, 15, 0, VGA_GREEN | (VGA_BLACK << 4));

            u32 vmcs_rev = virtualization_get_vmcs_revision_id();
            u32 vmcs_size = virtualization_get_vmcs_size();

            char vmcs_str[40];
            sprintf(vmcs_str, "VMCS: Rev %08X Size %u", vmcs_rev, vmcs_size);
            vga_puts_at(vmcs_str, 16, 0, VGA_LBLUE | (VGA_BLACK << 4));
        }

        if (virtualization_is_svm_supported()) {
            u32 svm_caps = virtualization_get_svm_capabilities();
            char caps_str[40] = "SVM: ";
            if (virtualization_has_npt()) strcat(caps_str, "NPT");
            vga_puts_at(caps_str, 15, 0, VGA_GREEN | (VGA_BLACK << 4));
        }

        u32 vm_entries = virtualization_get_vm_entries();
        u32 vm_exits = virtualization_get_vm_exits();

        char stats_str[40];
        sprintf(stats_str, "Entries: %u Exits: %u", vm_entries, vm_exits);
        vga_puts_at(stats_str, 17, 0, VGA_WHITE | (VGA_BLACK << 4));
    } else {
        vga_puts_at("Virtualization not supported", 13, 0, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(3000);
    }
}

static void iommu_demo_task(void) {
    vga_set_cursor(12, 40);
    vga_puts("IOMMU:\n");

    if (iommu_is_supported()) {
        char support_str[40] = "Support: ";
        if (iommu_is_intel_vtd_supported()) strcat(support_str, "VT-d ");
        if (iommu_is_amd_vi_supported()) strcat(support_str, "AMD-Vi");
        vga_puts_at(support_str, 13, 40, VGA_CYAN | (VGA_BLACK << 4));

        char enabled_str[40];
        sprintf(enabled_str, "Enabled: %s", iommu_is_enabled() ? "Yes" : "No");
        vga_puts_at(enabled_str, 14, 40, VGA_YELLOW | (VGA_BLACK << 4));

        char trans_str[40];
        sprintf(trans_str, "Translation: %s", iommu_is_translation_enabled() ? "On" : "Off");
        vga_puts_at(trans_str, 15, 40, VGA_GREEN | (VGA_BLACK << 4));

        u32 num_drhd = iommu_get_num_drhd_units();
        u32 domains = iommu_get_domain_count();
        u32 devices = iommu_get_device_count();

        char units_str[40];
        sprintf(units_str, "DRHD: %u Dom: %u Dev: %u", num_drhd, domains, devices);
        vga_puts_at(units_str, 16, 40, VGA_LBLUE | (VGA_BLACK << 4));

        u32 faults = iommu_get_translation_faults();
        u32 invalidations = iommu_get_invalidation_requests();

        char stats_str[40];
        sprintf(stats_str, "Faults: %u Inval: %u", faults, invalidations);
        vga_puts_at(stats_str, 17, 40, VGA_WHITE | (VGA_BLACK << 4));
    } else {
        vga_puts_at("IOMMU not supported", 13, 40, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(4000);
    }
}

static void performance_monitoring_demo_task(void) {
    vga_set_cursor(18, 0);
    vga_puts("Performance Monitoring:\n");

    if (performance_monitoring_is_supported()) {
        u8 version = performance_monitoring_get_version();
        u8 gp_counters = performance_monitoring_get_num_gp_counters();
        u8 fixed_counters = performance_monitoring_get_num_fixed_counters();

        char info_str[40];
        sprintf(info_str, "Ver: %u GP: %u Fixed: %u", version, gp_counters, fixed_counters);
        vga_puts_at(info_str, 19, 0, VGA_CYAN | (VGA_BLACK << 4));

        u8 gp_width = performance_monitoring_get_gp_counter_width();
        u8 fixed_width = performance_monitoring_get_fixed_counter_width();

        char width_str[40];
        sprintf(width_str, "Width: GP %ub Fixed %ub", gp_width, fixed_width);
        vga_puts_at(width_str, 20, 0, VGA_YELLOW | (VGA_BLACK << 4));

        u64 instructions = performance_monitoring_get_instructions_retired();
        u64 cycles = performance_monitoring_get_cpu_cycles();

        char perf_str[40];
        sprintf(perf_str, "Instr: %u Cycles: %u", (u32)instructions, (u32)cycles);
        vga_puts_at(perf_str, 21, 0, VGA_GREEN | (VGA_BLACK << 4));

        u32 ipc = performance_monitoring_calculate_ipc();
        u32 samples = performance_monitoring_get_total_samples();

        char stats_str[40];
        sprintf(stats_str, "IPC: %u.%03u Samples: %u", ipc / 1000, ipc % 1000, samples);
        vga_puts_at(stats_str, 22, 0, VGA_LBLUE | (VGA_BLACK << 4));

        u32 overflows = performance_monitoring_get_overflow_interrupts();
        u64 monitor_time = performance_monitoring_get_total_monitoring_time();

        char time_str[40];
        sprintf(time_str, "Overflows: %u Time: %u", overflows, (u32)monitor_time);
        vga_puts_at(time_str, 23, 0, VGA_WHITE | (VGA_BLACK << 4));
    } else {
        vga_puts_at("Performance monitoring not supported", 19, 0, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(2000);
    }
}

static void cpu_security_demo_task(void) {
    vga_set_cursor(12, 0);
    vga_puts("CPU Security:\n");

    if (cpu_security_is_supported()) {
        char support_str[40] = "Support: ";
        if (cpu_security_is_smep_supported()) strcat(support_str, "SMEP ");
        if (cpu_security_is_smap_supported()) strcat(support_str, "SMAP ");
        if (cpu_security_is_cet_supported()) strcat(support_str, "CET ");
        if (cpu_security_is_mpx_supported()) strcat(support_str, "MPX ");
        if (cpu_security_is_pku_supported()) strcat(support_str, "PKU");
        vga_puts_at(support_str, 13, 0, VGA_CYAN | (VGA_BLACK << 4));

        char enabled_str[40] = "Enabled: ";
        if (cpu_security_is_smep_enabled()) strcat(enabled_str, "SMEP ");
        if (cpu_security_is_smap_enabled()) strcat(enabled_str, "SMAP ");
        if (cpu_security_is_cet_user_enabled()) strcat(enabled_str, "CET-U ");
        if (cpu_security_is_cet_supervisor_enabled()) strcat(enabled_str, "CET-S ");
        if (cpu_security_is_mpx_enabled()) strcat(enabled_str, "MPX ");
        if (cpu_security_is_pku_enabled()) strcat(enabled_str, "PKU");
        vga_puts_at(enabled_str, 14, 0, VGA_YELLOW | (VGA_BLACK << 4));

        u32 total_violations = cpu_security_get_total_violations();
        u32 smep_violations = cpu_security_get_smep_violations();
        u32 smap_violations = cpu_security_get_smap_violations();

        char viol_str[40];
        sprintf(viol_str, "Violations: %u SMEP:%u SMAP:%u", total_violations, smep_violations, smap_violations);
        vga_puts_at(viol_str, 15, 0, VGA_GREEN | (VGA_BLACK << 4));

        u32 mpx_violations = cpu_security_get_mpx_violations();
        u32 pkey_violations = cpu_security_get_pkey_violations();
        u32 active_mitigations = cpu_security_get_active_mitigations();

        char sec_str[40];
        sprintf(sec_str, "MPX:%u PKU:%u Mitigations:%u", mpx_violations, pkey_violations, active_mitigations);
        vga_puts_at(sec_str, 16, 0, VGA_LBLUE | (VGA_BLACK << 4));

        if (cpu_security_is_pku_enabled()) {
            u32 pkru = cpu_security_get_pkru_value();
            char pkru_str[40];
            sprintf(pkru_str, "PKRU: %08X", pkru);
            vga_puts_at(pkru_str, 17, 0, VGA_WHITE | (VGA_BLACK << 4));
        }
    } else {
        vga_puts_at("CPU security not supported", 13, 0, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(3000);
    }
}

static void memory_protection_demo_task(void) {
    vga_set_cursor(12, 40);
    vga_puts("Memory Protection:\n");

    if (memory_protection_adv_is_supported()) {
        char support_str[40] = "Support: ";
        if (memory_protection_adv_is_nx_supported()) strcat(support_str, "NX ");
        if (memory_protection_adv_is_smep_active()) strcat(support_str, "SMEP ");
        if (memory_protection_adv_is_smap_active()) strcat(support_str, "SMAP ");
        if (memory_protection_adv_is_pku_active()) strcat(support_str, "PKU ");
        if (memory_protection_adv_is_cet_active()) strcat(support_str, "CET");
        vga_puts_at(support_str, 13, 40, VGA_CYAN | (VGA_BLACK << 4));

        u32 regions = memory_protection_adv_get_num_protected_regions();
        u32 violations = memory_protection_adv_get_num_violations();

        char region_str[40];
        sprintf(region_str, "Regions: %u Violations: %u", regions, violations);
        vga_puts_at(region_str, 14, 40, VGA_YELLOW | (VGA_BLACK << 4));

        u32 access_checks = memory_protection_adv_get_total_access_checks();
        u32 access_denied = memory_protection_adv_get_access_denied_count();

        char access_str[40];
        sprintf(access_str, "Checks: %u Denied: %u", access_checks, access_denied);
        vga_puts_at(access_str, 15, 40, VGA_GREEN | (VGA_BLACK << 4));

        u32 pkeys_used = memory_protection_adv_get_protection_keys_used();
        u64 last_violation = memory_protection_adv_get_last_violation_time();

        char pkey_str[40];
        sprintf(pkey_str, "PKeys: %08X Last: %u", pkeys_used, (u32)last_violation);
        vga_puts_at(pkey_str, 16, 40, VGA_LBLUE | (VGA_BLACK << 4));

        char domain_str[40];
        sprintf(domain_str, "Domains: K/U/D/S");
        vga_puts_at(domain_str, 17, 40, VGA_WHITE | (VGA_BLACK << 4));
    } else {
        vga_puts_at("Memory protection not supported", 13, 40, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(4000);
    }
}

static void cpu_debug_ext_demo_task(void) {
    vga_set_cursor(18, 40);
    vga_puts("CPU Debug Extensions:\n");

    if (cpu_debug_ext_is_supported()) {
        char support_str[40] = "Support: ";
        if (cpu_debug_ext_is_intel_pt_supported()) strcat(support_str, "PT ");
        if (cpu_debug_ext_is_lbr_supported()) strcat(support_str, "LBR ");
        if (cpu_debug_ext_is_bts_supported()) strcat(support_str, "BTS");
        vga_puts_at(support_str, 19, 40, VGA_CYAN | (VGA_BLACK << 4));

        char enabled_str[40] = "Enabled: ";
        if (cpu_debug_ext_is_intel_pt_enabled()) strcat(enabled_str, "PT ");
        if (cpu_debug_ext_is_lbr_enabled()) strcat(enabled_str, "LBR ");
        if (cpu_debug_ext_is_bts_enabled()) strcat(enabled_str, "BTS");
        vga_puts_at(enabled_str, 20, 40, VGA_YELLOW | (VGA_BLACK << 4));

        if (cpu_debug_ext_is_lbr_supported()) {
            u8 lbr_depth = cpu_debug_ext_get_lbr_depth();
            u32 lbr_records = cpu_debug_ext_get_lbr_records_captured();

            char lbr_str[40];
            sprintf(lbr_str, "LBR: Depth %u Records %u", lbr_depth, lbr_records);
            vga_puts_at(lbr_str, 21, 40, VGA_GREEN | (VGA_BLACK << 4));
        }

        u32 trace_sessions = cpu_debug_ext_get_total_trace_sessions();
        u64 trace_time = cpu_debug_ext_get_total_trace_time();

        char trace_str[40];
        sprintf(trace_str, "Sessions: %u Time: %u", trace_sessions, (u32)trace_time);
        vga_puts_at(trace_str, 22, 40, VGA_LBLUE | (VGA_BLACK << 4));

        u32 debug_interrupts = cpu_debug_ext_get_debug_interrupts();
        u32 bts_overflows = cpu_debug_ext_get_bts_buffer_overflows();

        char int_str[40];
        sprintf(int_str, "Interrupts: %u Overflows: %u", debug_interrupts, bts_overflows);
        vga_puts_at(int_str, 23, 40, VGA_WHITE | (VGA_BLACK << 4));
    } else {
        vga_puts_at("CPU debug extensions not supported", 19, 40, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(5000);
    }
}

static void cpu_instruction_ext_demo_task(void) {
    vga_set_cursor(12, 0);
    vga_puts("CPU Instruction Extensions:\n");

    if (cpu_instruction_ext_is_supported()) {
        char support_str[40] = "Support: ";
        if (cpu_instruction_ext_is_avx512_supported()) strcat(support_str, "AVX-512 ");
        if (cpu_instruction_ext_is_amx_supported()) strcat(support_str, "AMX ");
        if (cpu_instruction_ext_is_apx_supported()) strcat(support_str, "APX");
        vga_puts_at(support_str, 13, 0, VGA_CYAN | (VGA_BLACK << 4));

        char enabled_str[40] = "Enabled: ";
        if (cpu_instruction_ext_is_avx512_enabled()) strcat(enabled_str, "AVX-512 ");
        if (cpu_instruction_ext_is_amx_enabled()) strcat(enabled_str, "AMX ");
        if (cpu_instruction_ext_is_apx_enabled()) strcat(enabled_str, "APX");
        vga_puts_at(enabled_str, 14, 0, VGA_YELLOW | (VGA_BLACK << 4));

        if (cpu_instruction_ext_is_avx512_supported()) {
            u32 avx512_features = cpu_instruction_ext_get_avx512_features();
            u32 avx512_instructions = cpu_instruction_ext_get_avx512_instructions_executed();

            char avx512_str[40];
            sprintf(avx512_str, "AVX-512: Features %08X Instr %u", avx512_features, avx512_instructions);
            vga_puts_at(avx512_str, 15, 0, VGA_GREEN | (VGA_BLACK << 4));
        }

        if (cpu_instruction_ext_is_amx_supported()) {
            u8 tiles = cpu_instruction_ext_get_tile_registers_configured();
            u64 tile_size = cpu_instruction_ext_get_tile_data_size();

            char amx_str[40];
            sprintf(amx_str, "AMX: Tiles %u Size %u", tiles, (u32)tile_size);
            vga_puts_at(amx_str, 16, 0, VGA_LBLUE | (VGA_BLACK << 4));
        }

        u32 context_switches = cpu_instruction_ext_get_total_context_switches();
        u32 xsave_ops = cpu_instruction_ext_get_xsave_operations();

        char ctx_str[40];
        sprintf(ctx_str, "Context: %u XSAVE: %u", context_switches, xsave_ops);
        vga_puts_at(ctx_str, 17, 0, VGA_WHITE | (VGA_BLACK << 4));
    } else {
        vga_puts_at("CPU instruction extensions not supported", 13, 0, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(3000);
    }
}

static void system_mgmt_mode_demo_task(void) {
    vga_set_cursor(12, 40);
    vga_puts("System Management Mode:\n");

    if (system_mgmt_mode_is_supported()) {
        char status_str[40];
        sprintf(status_str, "Status: %s %s",
                system_mgmt_mode_is_enabled() ? "Enabled" : "Disabled",
                system_mgmt_mode_is_locked() ? "Locked" : "Unlocked");
        vga_puts_at(status_str, 13, 40, VGA_CYAN | (VGA_BLACK << 4));

        u64 smram_base = system_mgmt_mode_get_smram_base();
        u32 smram_size = system_mgmt_mode_get_smram_size();

        char smram_str[40];
        sprintf(smram_str, "SMRAM: %08X Size: %uKB", (u32)smram_base, smram_size / 1024);
        vga_puts_at(smram_str, 14, 40, VGA_YELLOW | (VGA_BLACK << 4));

        u32 smi_count = system_mgmt_mode_get_total_smi_count();
        u32 smm_entries = system_mgmt_mode_get_smm_entries();

        char smi_str[40];
        sprintf(smi_str, "SMI Count: %u Entries: %u", smi_count, smm_entries);
        vga_puts_at(smi_str, 15, 40, VGA_GREEN | (VGA_BLACK << 4));

        u32 smi_sources = system_mgmt_mode_get_smi_sources_enabled();
        u8 handler_installed = system_mgmt_mode_is_handler_installed();

        char handler_str[40];
        sprintf(handler_str, "Sources: %08X Handler: %s", smi_sources, handler_installed ? "Yes" : "No");
        vga_puts_at(handler_str, 16, 40, VGA_LBLUE | (VGA_BLACK << 4));

        u64 smm_time = system_mgmt_mode_get_total_smm_time();
        u32 smm_revision = system_mgmt_mode_get_smm_revision();

        char time_str[40];
        sprintf(time_str, "Time: %u Rev: %08X", (u32)smm_time, smm_revision);
        vga_puts_at(time_str, 17, 40, VGA_WHITE | (VGA_BLACK << 4));
    } else {
        vga_puts_at("System Management Mode not supported", 13, 40, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(4000);
    }
}

static void thermal_mgmt_demo_task(void) {
    vga_set_cursor(18, 0);
    vga_puts("Thermal Management:\n");

    if (thermal_mgmt_is_supported()) {
        char support_str[40] = "Support: ";
        if (thermal_mgmt_is_thermal_monitoring_supported()) strcat(support_str, "Thermal ");
        if (thermal_mgmt_is_rapl_supported()) strcat(support_str, "RAPL");
        vga_puts_at(support_str, 19, 0, VGA_CYAN | (VGA_BLACK << 4));

        u32 current_temp = thermal_mgmt_get_current_temperature();
        u32 target_temp = thermal_mgmt_get_thermal_target();
        u32 critical_temp = thermal_mgmt_get_critical_temperature();

        char temp_str[40];
        sprintf(temp_str, "Temp: %u°C Target: %u°C Crit: %u°C", current_temp, target_temp, critical_temp);
        vga_puts_at(temp_str, 20, 0, VGA_YELLOW | (VGA_BLACK << 4));

        char status_str[40] = "Status: ";
        if (thermal_mgmt_is_thermal_throttling_active()) strcat(status_str, "THROTTLE ");
        if (thermal_mgmt_is_prochot_active()) strcat(status_str, "PROCHOT ");
        if (thermal_mgmt_is_critical_temp_reached()) strcat(status_str, "CRITICAL");
        if (strlen(status_str) == 8) strcat(status_str, "Normal");
        vga_puts_at(status_str, 21, 0, VGA_GREEN | (VGA_BLACK << 4));

        u32 throttle_count = thermal_mgmt_get_throttle_count();
        u32 thermal_events = thermal_mgmt_get_thermal_events_count();

        char event_str[40];
        sprintf(event_str, "Throttles: %u Events: %u", throttle_count, thermal_events);
        vga_puts_at(event_str, 22, 0, VGA_LBLUE | (VGA_BLACK << 4));

        if (thermal_mgmt_is_rapl_supported()) {
            const rapl_info_t* rapl = thermal_mgmt_get_rapl_info();
            char rapl_str[40];
            sprintf(rapl_str, "RAPL: PL1 %uW PL2 %uW", rapl->pkg_power_limit_1, rapl->pkg_power_limit_2);
            vga_puts_at(rapl_str, 23, 0, VGA_WHITE | (VGA_BLACK << 4));
        }
    } else {
        vga_puts_at("Thermal management not supported", 19, 0, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(2000);
    }
}

static void hwcrypto_demo_task(void) {
    hwcrypto_init();

    vga_set_cursor(18, 40);
    vga_puts("Hardware Crypto:\n");

    if (hwcrypto_is_supported()) {
        char caps_str[40] = "Support: ";
        if (hwcrypto_has_aesni()) strcat(caps_str, "AES ");
        if (hwcrypto_has_shani()) strcat(caps_str, "SHA ");
        if (hwcrypto_has_rdrand()) strcat(caps_str, "RNG");
        vga_puts_at(caps_str, 19, 40, VGA_CYAN | (VGA_BLACK << 4));

        /* Run self-test */
        u8 self_test = hwcrypto_self_test();
        char test_str[40];
        sprintf(test_str, "Self-test: %s", self_test ? "PASS" : "FAIL");
        vga_puts_at(test_str, 20, 40, self_test ? VGA_GREEN : VGA_RED | (VGA_BLACK << 4));

        /* Show benchmarks */
        if (hwcrypto_has_aesni()) {
            u32 aes_cycles = hwcrypto_benchmark_aes(1000);
            char aes_str[40];
            sprintf(aes_str, "AES: %u cyc/block", aes_cycles);
            vga_puts_at(aes_str, 21, 40, VGA_WHITE | (VGA_BLACK << 4));
        }

        if (hwcrypto_has_shani()) {
            u32 sha_cycles = hwcrypto_benchmark_sha256(1000);
            char sha_str[40];
            sprintf(sha_str, "SHA-256: %u cyc/hash", sha_cycles);
            vga_puts_at(sha_str, 22, 40, VGA_WHITE | (VGA_BLACK << 4));
        }

        /* Show RNG status */
        if (hwcrypto_has_rdrand()) {
            u32 random = hwcrypto_rdrand32();
            char rng_str[40];
            sprintf(rng_str, "RDRAND: %08X", random);
            vga_puts_at(rng_str, 23, 40, VGA_YELLOW | (VGA_BLACK << 4));
        }

        /* Show stats */
        const hwcrypto_stats_t* stats = hwcrypto_get_stats();
        if (stats->aes_ops_count > 0) {
            char stats_str[40];
            sprintf(stats_str, "AES ops: %u RNG: %u", stats->aes_ops_count, stats->rng_requests);
            vga_puts_at(stats_str, 24, 40, VGA_GRAY | (VGA_BLACK << 4));
        }
    } else {
        vga_puts_at("Hardware crypto not supported", 19, 40, VGA_RED | (VGA_BLACK << 4));
    }

    while (1) {
        timer_delay(3000);
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
    task_create(atomic_demo_task, 13, 256);
    task_create(cpu_features_demo_task, 14, 256);
    task_create(cpuid_ext_demo_task, 15, 256);
    task_create(errata_demo_task, 16, 256);
    task_create(numa_demo_task, 17, 256);
    task_create(perfmon_demo_task, 18, 256);
    task_create(x2apic_demo_task, 19, 256);
    task_create(iommu_demo_task, 20, 256);
    task_create(acpi_demo_task, 21, 256);
    task_create(apic_demo_task, 22, 256);
    task_create(usb_demo_task, 23, 256);
    task_create(audio_demo_task, 24, 256);
    task_create(network_demo_task, 25, 256);
    task_create(dma_demo_task, 26, 256);
    task_create(hpet_demo_task, 27, 256);
    task_create(msr_demo_task, 28, 256);
    task_create(thermal_demo_task, 29, 256);
    task_create(power_demo_task, 30, 256);
    task_create(cache_demo_task, 31, 256);
    task_create(vmx_demo_task, 32, 256);
    task_create(longmode_demo_task, 33, 256);
    task_create(microcode_demo_task, 34, 256);
    task_create(topology_demo_task, 35, 256);
    task_create(xsave_demo_task, 36, 256);
    task_create(security_demo_task, 37, 256);
    task_create(debug_demo_task, 38, 256);
    task_create(cpufreq_demo_task, 39, 256);
    task_create(ioapic_demo_task, 40, 256);
    task_create(cache_mgmt_demo_task, 41, 256);
    task_create(memory_mgmt_demo_task, 42, 256);
    task_create(simd_demo_task, 43, 256);
    task_create(interrupt_mgmt_demo_task, 44, 256);
    task_create(power_mgmt_demo_task, 45, 256);
    task_create(memory_protection_demo_task, 46, 256);
    task_create(virt_ext_demo_task, 47, 256);
    task_create(debug_hw_demo_task, 48, 256);
    task_create(pmu_ext_demo_task, 49, 256);
    task_create(trace_hw_demo_task, 50, 256);
    task_create(exception_mgmt_demo_task, 51, 256);
    task_create(memory_adv_demo_task, 52, 256);
    task_create(security_ext_demo_task, 53, 256);
    task_create(freq_scaling_demo_task, 54, 256);
    task_create(system_ctrl_demo_task, 55, 256);
    task_create(cpu_ext_demo_task, 56, 256);
    task_create(microarch_demo_task, 57, 256);
    task_create(apic_ext_demo_task, 58, 256);
    task_create(timing_sync_demo_task, 59, 256);
    task_create(cache_coherency_demo_task, 60, 256);
    task_create(memory_ordering_demo_task, 61, 256);
    task_create(power_states_demo_task, 62, 256);
    task_create(cpu_topology_demo_task, 63, 256);
    task_create(interrupt_routing_demo_task, 64, 256);
    task_create(cpu_errata_demo_task, 65, 256);
    task_create(virtualization_demo_task, 66, 256);
    task_create(iommu_demo_task, 67, 256);
    task_create(performance_monitoring_demo_task, 68, 256);
    task_create(cpu_security_demo_task, 69, 256);
    task_create(memory_protection_demo_task, 70, 256);
    task_create(cpu_debug_ext_demo_task, 71, 256);
    task_create(cpu_instruction_ext_demo_task, 72, 256);
    task_create(system_mgmt_mode_demo_task, 73, 256);
    task_create(thermal_mgmt_demo_task, 74, 256);
    task_create(hwcrypto_demo_task, 75, 256);
    task_create(system_info_task, 76, 256);
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
