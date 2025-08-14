# x86-64 PC Support

**Intel/AMD x86-64 PC with standard hardware**

## Features
- **VGA Text Mode**: 80x25 color display @ 0xB8000
- **PS/2 Keyboard**: Full scan code translation with modifiers
- **PCI Bus**: Device enumeration and configuration
- **PIT Timer**: 8253/8254 programmable interval timer @ 1 kHz
- **ATA/IDE**: Hard disk and CD-ROM support
- **PC Speaker**: Beep generation via PIT channel 2

## Memory Map
- **VGA Buffer**: 0xB8000 (32 kB)
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
