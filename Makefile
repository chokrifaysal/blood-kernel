###############################################################################
# BLOOD_KERNEL – Universal Makefile 
# Targets:
#   arm (STM32F4), esp32s3 (RISC-V), pic32mz (MIPS), rp2040, rt1170,
#   tms570, avr128da, gd32vf103, ra6m5, rpi4, stm32h, x86
###############################################################################

# ---------- USER SELECT ----------
ARCH ?= stm32f4          # default

# ---------- TOOLCHAIN SELECTION ----------
ifeq ($(ARCH),stm32f4)
    CROSS    := arm-none-eabi-
    LD_SCRIPT := arch/stm32f4/linker.ld
    CFLAGS   := -D__arm__ -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -O2

else ifeq ($(ARCH),stm32h)
    CROSS    := arm-none-eabi-
    LD_SCRIPT := arch/stm32h745/linker_cm7.ld
    CFLAGS   := -D__arm__ -mcpu=cortex-m7 -mfloat-abi=hard -mfpu=fpv5-d16 -O2

else ifeq ($(ARCH),tms570)
    CROSS    := arm-none-eabi-
    LD_SCRIPT := arch/tms570/linker.ld
    CFLAGS   := -D__arm__ -mcpu=cortex-r4f -mfloat-abi=hard -mfpu=vfpv3-d16 -O2

else ifeq ($(ARCH),rp2040)
    CROSS    := arm-none-eabi-
    LD_SCRIPT := arch/rp2040/linker.ld
    CFLAGS   := -D__arm__ -mcpu=cortex-m0plus -mthumb -O2

else ifeq ($(ARCH),ra6m5)
    CROSS    := arm-none-eabi-
    LD_SCRIPT := arch/ra6m5/linker.ld
    CFLAGS   := -D__arm__ -mcpu=cortex-m33 -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O2

else ifeq ($(ARCH),rt1170)
    CROSS    := arm-none-eabi-
    LD_SCRIPT := arch/rt1170/linker.ld
    CFLAGS   := -D__arm__ -mcpu=cortex-m7 -mfloat-abi=hard -mfpu=fpv5-d16 -O2

else ifeq ($(ARCH),gd32vf103)
    CROSS    := riscv64-unknown-elf-
    LD_SCRIPT := arch/gd32vf103/linker.ld
    CFLAGS   := -D__riscv -march=rv32imc -mabi=ilp32 -O2

else ifeq ($(ARCH),esp32s3)
    CROSS    := riscv64-unknown-elf-
    LD_SCRIPT := arch/esp32s3/linker.ld
    CFLAGS   := -D__riscv -march=rv32imc -mabi=ilp32 -O2

else ifeq ($(ARCH),rpi4)
    CROSS    := aarch64-linux-gnu-
    LD_SCRIPT := arch/rpi4/linker.ld
    CFLAGS   := -D__aarch64__ -mcpu=cortex-a72 -O2

else ifeq ($(ARCH),pic32mz)
    CROSS    := mips-mti-elf-
    LD_SCRIPT := arch/pic32mz/linker.ld
    CFLAGS   := -D__mips__ -march=mips32r5 -mabi=32 -O2

else ifeq ($(ARCH),avr128da)
    CROSS    := avr-
    LD_SCRIPT := arch/avr128da/linker.ld
    CFLAGS   := -D__AVR_ARCH__ -mmcu=avr128da48 -Os -ffreestanding

else ifeq ($(ARCH),x86)
    CROSS    := i686-elf-
    LD_SCRIPT := arch/x86/linker.ld
    CFLAGS   := -D__x86_64__ -m32 -ffreestanding -nostdlib -O2

endif

CC      := $(CROSS)gcc
LD      := $(CROSS)ld
OBJCOPY := $(CROSS)objcopy
OBJDUMP := $(CROSS)objdump
QEMU    := qemu-system-i386

# ---------- COMMON FLAGS ----------
CFLAGS  += -Wall -Wextra -Werror -std=c11 -g
CFLAGS  += -ffreestanding -nostdlib -nostartfiles
CFLAGS  += -Iinclude -Iarch/$(ARCH)

# ---------- OBJECTS ----------
KERNEL_OBJS := $(wildcard src/kernel/*.c) $(wildcard arch/$(ARCH)/*.c) $(wildcard arch/$(ARCH)/*.S)

# ---------- BUILD RULES ----------
.PHONY: all clean flash qemu help

all: build/kernel.elf

# ---------- PER-ARCH BUILD ----------
build/kernel.elf: $(KERNEL_OBJS)
	@mkdir -p build
	$(CC) $(CFLAGS) -T $(LD_SCRIPT) -o $@ $^
	$(OBJCOPY) -O binary $@ build/kernel.bin

# ---------- FLASH / QEMU ----------
flash:
	@if [ "$(ARCH)" = "x86" ]; then \
	    $(QEMU) -kernel build/kernel.elf -serial stdio -s -S & \
	    echo "QEMU started on stdio"; \
	elif [ -x tools/flash_$(ARCH).sh ]; then \
	    ./tools/flash_$(ARCH).sh; \
	else \
	    echo "No flash script for $(ARCH)"; \
	fi

qemu:
	@if [ "$(ARCH)" = "x86" ]; then \
	    $(QEMU) -kernel build/kernel.elf -serial stdio -s -S; \
	else \
	    echo "qemu only for x86"; \
	fi

# ---------- CLEAN ----------
clean:
	rm -rf build

# ---------- HELP ----------
help:
	@echo "Supported ARCH:"
	@echo "  stm32f4  stm32h  tms570  rp2040  ra6m5"
	@echo "  rt1170   gd32vf103  esp32s3  rpi4"
	@echo "  pic32mz  avr128da  x86"
	@echo
	@echo "Usage:"
	@echo "  make ARCH=<target>"
	@echo "  make ARCH=<target> flash"
