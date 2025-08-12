# BLOOD_KERNEL v1.2 – multi-arch

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
