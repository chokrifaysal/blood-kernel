# BLOOD_KERNEL v1.2 – multi-arch

ifeq ($(ARCH),gd32vf103)
    CROSS := riscv64-unknown-elf-
    LD_SCRIPT := arch/gd32vf103/linker.ld
    CFLAGS += -D__riscv -march=rv32imc -mabi=ilp32 -O2
endif
# RT1170 build
ifeq ($(ARCH),rt1170)
    CROSS := arm-none-eabi-
    LD_SCRIPT := arch/rt1170/linker.ld
    CFLAGS += -D__arm__ -mcpu=cortex-m7 -mfloat-abi=hard -mfpu=fpv5-d16 -O2
endif

# RA6M5 build
ifeq ($(ARCH),ra6m5)
    CROSS := arm-none-eabi-
    LD_SCRIPT := arch/ra6m5/linker.ld
    CFLAGS += -D__arm__ -mcpu=cortex-m33 -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O2
endif
# AVR128DA48 build
ifeq ($(ARCH),avr128da)
    CROSS := avr-
    LD_SCRIPT := arch/avr128da/linker.ld
    CFLAGS += -D__AVR_ARCH__ -mmcu=avr128da48 -Os -ffreestanding
    OBJCOPY := $(CROSS)objcopy
    OBJCOPY_OPT := -O ihex
endif
# PIC32MZ build added
ifeq ($(ARCH),pic32mz)
    CROSS := mips-mti-elf-
    LD_SCRIPT := arch/pic32mz/linker.ld
    CFLAGS += -D__mips__ -march=mips32r5 -mabi=32
endif
# ESP32-S3 RISC-V build
ifeq ($(ARCH),esp32s3)
    CROSS := riscv64-unknown-elf-
    LD_SCRIPT := arch/esp32s3/linker.ld
    CFLAGS += -D__riscv -march=rv32imc -mabi=ilp32
endif
# RP2040 build added
ifeq ($(ARCH),rp2040)
    CROSS := arm-none-eabi-
    LD_SCRIPT := arch/rp2040/linker.ld
    CFLAGS += -D__arm__ -mcpu=cortex-m0plus
endif
ifeq ($(ARCH),rpi4)
    CROSS := aarch64-linux-gnu-
    LD_SCRIPT := arch/rpi4/linker.ld
    CFLAGS += -D__aarch64__
else ifeq ($(ARCH),stm32)
    CROSS := arm-none-eabi-
    LD_SCRIPT := arch/arm/cortex-m/linker.ld
    CFLAGS += -D__arm__
else ifeq ($(ARCH),x86)
    CROSS := i686-elf-
    LD_SCRIPT := arch/x86/linker.ld
    CFLAGS += -D__x86_64__
endif

CC := $(CROSS)gcc
LD := $(CROSS)ld

.PHONY: rpi4 stm32 x86 clean
rpi4:  ARCH=rpi4  all
stm32: ARCH=stm32 all
x86:   ARCH=x86   all

all: build/kernel.elf
build/kernel.elf: $(wildcard src/kernel/*.c arch/$(ARCH)/*.S)
	@mkdir -p build
	$(CC) $(CFLAGS) -T $(LD_SCRIPT) -o $@ $^

clean:
	rm -rf build
