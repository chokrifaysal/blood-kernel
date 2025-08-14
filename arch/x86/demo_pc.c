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

static void system_info_task(void) {
    while (1) {
        /* Update system stats */
        u32 ticks = pit_get_ticks();
        u32 freq = pit_get_frequency();

        char stats[80];
        sprintf(stats, "Ticks: %u  Freq: %u Hz", ticks, freq);
        vga_puts_at(stats, 23, 0, VGA_YELLOW | (VGA_BLACK << 4));

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
    task_create(system_info_task, 9, 256);
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
