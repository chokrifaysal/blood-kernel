# x86-64 PC Support

**Intel/AMD x86-64 PC with standard hardware**

## Features
- **IDT**: 256-entry Interrupt Descriptor Table with exception handling
- **PIC**: 8259A Programmable Interrupt Controller with IRQ management
- **MMU**: 4KB page-based memory management with demand paging
- **CPUID**: CPU identification and feature detection
- **RTC**: MC146818 Real-Time Clock with CMOS memory access
- **Serial**: 16550 UART COM1/COM2/COM3/COM4 with interrupt buffering
- **Floppy**: 82077AA controller for 1.44MB/720KB/1.2MB/360KB drives
- **VGA Text Mode**: 80x25 color display @ 0xB8000
- **PS/2 Keyboard**: Full scan code translation with modifiers
- **PCI Bus**: Device enumeration and configuration
- **PIT Timer**: 8253/8254 programmable interval timer @ 1 kHz
- **ATA/IDE**: Hard disk and CD-ROM support
- **PC Speaker**: Beep generation via PIT channel 2

## Memory Map
- **Kernel**: 0x00100000-0x00400000 (3 MB)
- **Page Directory**: 4 KB aligned
- **Page Tables**: 4 KB each
- **Heap**: 0xD0000000-0xE0000000 (256 MB)
- **VGA Buffer**: 0xB8000 (32 kB)
- **CMOS/RTC**: 0x70/0x71
- **COM1**: 0x3F8-0x3FF (IRQ4)
- **COM2**: 0x2F8-0x2FF (IRQ3)
- **Floppy**: 0x3F0-0x3F7 (IRQ6, DMA2)
- **PIC**: 0x20/0x21, 0xA0/0xA1
- **PCI Config**: 0xCF8/0xCFC
- **PIT**: 0x40-0x43
- **PS/2**: 0x60/0x64
- **PC Speaker**: 0x61

## Build & Boot
```bash
make ARCH=x86
qemu-system-x86_64 -kernel build/kernel.bin
```

## Hardware Drivers

### Interrupt System
- IDT with 256 entries for exceptions and IRQs
- Exception handlers for all x86 faults
- IRQ routing through 8259A PIC
- Atomic operations and memory barriers

### Memory Management
- 4KB page-based virtual memory
- Identity mapping for kernel space
- Higher-half kernel at 0xC0000000
- Demand paging for heap allocation
- Physical page bitmap allocator

### CPU Identification
- CPUID instruction support detection
- Vendor string (Intel, AMD, etc.)
- CPU brand string and model info
- Feature flags (SSE, AVX, etc.)
- TSC and RDRAND support

### Real-Time Clock
- MC146818 RTC with battery backup
- Date/time reading and setting
- Alarm functionality with interrupts
- CMOS memory access (128 bytes)
- System configuration storage

### Serial Ports
- 16550 UART with FIFO buffers
- COM1/COM2/COM3/COM4 support
- Configurable baud rates up to 115200
- Hardware flow control (RTS/CTS/DTR)
- Interrupt-driven I/O with buffering

### Floppy Disk Controller
- 82077AA compatible controller
- Support for 1.44MB, 720KB, 1.2MB, 360KB
- DMA-based sector read/write
- Drive detection via CMOS
- CHS addressing with seek optimization

### VGA Text Mode
- 80x25 character display
- 16 foreground/background colors
- Hardware cursor control
- Box drawing and formatting

### PS/2 Keyboard
- Scan code set 1 translation
- Modifier key tracking (Shift, Ctrl, Alt, Caps)
- LED control (Caps, Num, Scroll)
- Interrupt-driven input buffer

### PCI Bus
- Configuration space access
- Device enumeration
- Vendor/device ID lookup
- Class code identification
- BAR (Base Address Register) parsing

### PIT Timer
- 1.193182 MHz base frequency
- Channel 0: System tick @ 1 kHz
- Channel 2: PC speaker control
- Microsecond precision delays
- Uptime tracking

### ATA/IDE
- Primary/secondary controllers
- Master/slave drives
- LBA28 addressing
- Sector read/write operations
- Drive identification (model, serial, capacity)

## API Examples

### VGA Display
```c
vga_init();
vga_set_color(VGA_WHITE, VGA_BLUE);
vga_puts("Hello World!");
vga_draw_box(10, 10, 20, 5, VGA_YELLOW);
```

### Keyboard Input
```c
ps2_kbd_init();
char c = ps2_kbd_getchar_blocking();
u8 mods = ps2_kbd_get_modifiers();
```

### PCI Enumeration
```c
pci_init();
pci_device_t* vga = pci_find_class(0x03, 0x00);
pci_enable_device(vga);
```

### Disk I/O
```c
ata_init();
u8 buffer[512];
ata_read_sectors(0, 0, 1, buffer); // Read MBR
```

### Timer Functions
```c
pit_init(1000); // 1 kHz
pit_delay(1000); // 1 second
pit_beep(440, 500); // A4 note for 500ms
```

## Demo Tasks
- **VGA Demo**: Color test and UI elements
- **Keyboard Test**: Interactive input with modifier display
- **PCI Scanner**: Lists all detected PCI devices
- **ATA Detection**: Shows connected drives and capacity
- **Timer Display**: Real-time uptime counter
- **System Info**: Live system statistics

## Performance
- **VGA**: Direct memory access, no GPU acceleration
- **Keyboard**: ~100 Hz scan rate, interrupt-driven
- **PCI**: 33 MHz bus, configuration cycles
- **Timer**: 1 MHz resolution, 32-bit counters
- **ATA**: PIO mode, ~16 MB/s transfer rate
