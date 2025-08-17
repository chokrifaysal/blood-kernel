# BLOOD_KERNEL

**bare metal microkernel that actually works**

A no-bullshit operating system kernel that boots on real hardware and doesn't waste your time with enterprise patterns or framework bloat. Written in C with inline assembly where it matters.

## What you get

- **Multi-architecture support**: x86 PC, ARM Cortex-M, RISC-V, MIPS, AVR
- **Real hardware drivers**: 54+ drivers for UART, GPIO, CAN, SPI, I²C, Ethernet, USB, Audio
- **Advanced x86 features**: SIMD, virtualization, power management, security extensions
- **Automotive-grade code**: MISRA-compliant, no dynamic allocation, bounds checking
- **Actual testing**: Runs on QEMU and real dev boards (STM32, TI TMS570, NXP i.MX, Raspberry Pi)

## Quick start

```bash
# Boot x86 in QEMU
make ARCH=x86 && make qemu

# Flash to STM32F4 board  
make ARCH=stm32f4 && make flash

# Build for automotive TMS570
make ARCH=tms570
```

## Supported platforms

| Architecture | Boards | Status |
|--------------|--------|--------|
| **x86** | PC, QEMU | Full support with advanced features |
| **ARM Cortex-M** | STM32F4/H7, TMS570, RP2040, RT1170, RA6M5 | Production ready |
| **ARM Cortex-A** | Raspberry Pi 4 | Basic support |
| **RISC-V** | ESP32-S3, GD32VF103 | Working |
| **MIPS** | PIC32MZ | Working |
| **AVR** | AVR128DA | Working |

## What makes this different

**No framework cancer**: Direct hardware access, no HAL layers hiding what's actually happening.

**Real implementations**: The x86 port has proper SIMD optimization, hardware debugging, Intel PT tracing, and virtualization support. Not just "hello world" demos.

**Predictable behavior**: Static memory allocation, deterministic timing, no hidden malloc() calls.

**Readable code**: 80-column limit, tabs not spaces, comments explain *why* not *what*.

## Build requirements

- **x86**: `i686-elf-gcc`, `qemu-system-i386`
- **ARM**: `arm-none-eabi-gcc`, `openocd`
- **RISC-V**: `riscv64-unknown-elf-gcc`
- **MIPS**: `mips-elf-gcc`
- **AVR**: `avr-gcc`

## Memory layout

```
x86:    0x00100000 - 0x00FFFFFF  (16MB kernel space)
ARM:    0x08000000 FLASH 512K, 0x20000000 RAM 128K
RISC-V: 0x08000000 FLASH 256K, 0x20000000 RAM 64K
```

No virtual memory tricks during boot - flat mapping for simplicity.

## Boot sequence

1. **x86**: GRUB multiboot → `boot.S` → `kernel_main()`
2. **ARM**: Reset vector → `startup.S` → `kernel_main()`
3. **RISC-V**: Reset vector → `boot.S` → `kernel_main()`

First actions:
- Detect available RAM
- Initialize UART (115200 8N1)
- Print system info
- Start scheduler

## Code style

- **Language**: C11 with inline assembly
- **Memory**: No heap, only stack and static allocation
- **Formatting**: 80 columns max, 8-space tabs, no camelCase
- **Comments**: Explain design decisions, not syntax

## Contributing

PRs welcome if you:
- Keep it simple and close to the metal
- Test on real hardware when possible
- Follow the existing code style
- Don't add dependencies or frameworks

## License
public domain / 0BSD / AGPL-3.0 / whatevah

This ensures any network-deployed modifications remain open source.
