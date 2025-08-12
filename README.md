# BLOOD_KERNEL

**minimal RT microkernel for PC/ARM/automotive**
no fucking around, no 10MB of headers, no "modern C++" cancer

## What this is?
- 100% bare metal, no libc, no exceptions, no templates
- boots on x86 (multiboot) and ARM Cortex-M (reset vector)
- automotive-grade MISRA-ish rules (no dynamic alloc, all bounds checked)
- real drivers: UART, GPIO, CAN, SPI, I²C, Ethernet
- QEMU + real HW (STM32F4, TI TMS570, NXP i.MX)

## Build it:
```bash
# x86 PC / QEMU
make x86 && make qemu

# ARM Cortex-M4 (STM32F407)
make arm && openocd -f board/st_nucleo_f4.cfg

# automotive dev board
make BOARD=tms570
```

## Needs (build-tools):
`i686-elf-gcc`, `arm-none-eabi-gcc`, `qemu-system-i386`, `openocd`

## Mem map:
- x86:  0x00100000 - 0x00FFFFFF  (16 MB kernel space)
- ARM:  0x08000000 FLASH 512K   0x20000000 RAM 128K
no MMU tricks yet, flat mapping for bring-up.

## Boot flow:
- x86:  GRUB → multiboot → boot.S → kernel_main()
- ARM:  reset vector → startup.S → kernel_main()

first thing kernel does:
- detect RAM via multiboot / system control block
- init UART0 (115200 8N1)
- print banner, jump to scheduler stub

## Coding rules (Check this if u’re tryna help build this repo UwU)
- C11 + hand-written asm only
- no heap, only static/global + stack
- 80 col hard limit, tabs=8, no camelCase
- comments explain why, not what

## License
public domain / 0BSD / AGPL-3.0 / whatevah

** PRs welcome, but keep it metal **
