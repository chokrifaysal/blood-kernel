# BLOOD_KERNEL Makefile - just builds
# Targets: x86-qemu, arm-cortex-m

CROSS_x86 := i686-elf-
CROSS_arm := arm-none-eabi-

CC_x86 := $(CROSS_x86)gcc
LD_x86 := $(CROSS_x86)ld
OBJCOPY_x86 := $(CROSS_x86)objcopy

CC_arm := $(CROSS_arm)gcc
LD_arm := $(CROSS_arm)ld
OBJCOPY_arm := $(CROSS_arm)objcopy

QEMU := qemu-system-i386

# Flags - keep it simple, no bloat
CFLAGS_x86 := -m32 -ffreestanding -nostdlib -nostartfiles \
              -Wall -Wextra -O2 -g -std=c99 \
              -Iinclude -Iarch/x86

CFLAGS_arm := -mcpu=cortex-m4 -mthumb -ffreestanding -nostdlib \
              -nostartfiles -Wall -Wextra -O2 -g -std=c99 \
              -Iinclude -Iarch/arm/cortex-m

# Build targets
.PHONY: all x86 arm clean qemu

all: x86

x86: build/kernel_x86.elf

arm: build/kernel_arm.elf

build/kernel_x86.elf: arch/x86/boot.o src/kernel/main.o
	@mkdir -p build
	$(LD_x86) -T arch/x86/linker.ld -o $@ $^

build/kernel_arm.elf: arch/arm/cortex-m/startup.o src/kernel/main.o
	@mkdir -p build
	$(LD_arm) -T arch/arm/cortex-m/linker.ld -o $@ $^

# Object files
arch/x86/boot.o: arch/x86/boot.S
	$(CC_x86) $(CFLAGS_x86) -c $< -o $@

arch/arm/cortex-m/startup.o: arch/arm/cortex-m/startup.S
	$(CC_arm) $(CFLAGS_arm) -c $< -o $@

src/kernel/main.o: src/kernel/main.c
	$(CC_x86) $(CFLAGS_x86) -c $< -o $@  # x86 build
	#$(CC_arm) $(CFLAGS_arm) -c $< -o $@  # arm build - fix later

# QEMU for testing
qemu: x86
	$(QEMU) -kernel build/kernel_x86.elf -serial stdio

clean:
	rm -rf build/*.o build/*.elf build/*.bin
