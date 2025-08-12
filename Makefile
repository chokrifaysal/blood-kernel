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

# Flags - kiss, no bloat
CFLAGS_x86 := -m32 -ffreestanding -nostdlib -nostartfiles \
              -Wall -Wextra -O2 -g -std=c99 \
              -Iinclude -Iarch/x86 \
              -D__x86_64__

CFLAGS_arm := -mcpu=cortex-m4 -mthumb -ffreestanding -nostdlib \
              -nostartfiles -Wall -Wextra -O2 -g -std=c99 \
              -Iinclude -Iarch/arm/cortex-m \
              -D__arm__

# Build targets
.PHONY: all x86 arm clean qemu task

all: x86

x86: build/kernel_x86.elf

arm: build/kernel_arm.elf

task: build/user_task.bin

build/kernel_x86.elf: arch/x86/boot.o arch/x86/context_switch.o \
                      src/kernel/main.o src/kernel/uart.o src/kernel/mem.o \
                      src/kernel/sched.o src/kernel/timer.o src/kernel/gpio.o \
                      src/kernel/spinlock.o src/kernel/msg.o src/kernel/can.o \
                      src/kernel/mpu.o src/kernel/elf.o src/kernel/string.o \
                      src/kernel/wdog.o src/kernel/kprintf.o
	@mkdir -p build
	$(LD_x86) -T arch/x86/linker.ld -o $@ $^

build/kernel_arm.elf: arch/arm/cortex-m/startup.o arch/arm/cortex-m/context_switch.o \
                      src/kernel/main.o src/kernel/uart.o src/kernel/mem.o \
                      src/kernel/sched.o src/kernel/timer.o src/kernel/gpio.o \
                      src/kernel/spinlock.o src/kernel/msg.o src/kernel/can.o \
                      src/kernel/mpu.o src/kernel/elf.o src/kernel/string.o \
                      src/kernel/wdog.o src/kernel/kprintf.o
	@mkdir -p build
	$(LD_arm) -T arch/arm/cortex-m/linker.ld -o $@ $^

build/user_task.bin: tasks/user_task.c tasks/task.ld
	@mkdir -p build
	$(CC_arm) -mcpu=cortex-m4 -mthumb -ffreestanding -nostdlib \
	          -Wl,-T,tasks/task.ld -Wl,-e,task_main -O2 -g -o build/user_task.elf $<
	$(OBJCOPY) -O binary build/user_task.elf $@

# Object files
arch/x86/boot.o: arch/x86/boot.S
	$(CC_x86) $(CFLAGS_x86) -c $< -o $@

arch/x86/context_switch.o: arch/x86/context_switch.S
	$(CC_x86) $(CFLAGS_x86) -c $< -o $@

arch/arm/cortex-m/startup.o: arch/arm/cortex-m/startup.S
	$(CC_arm) $(CFLAGS_arm) -c $< -o $@

arch/arm/cortex-m/context_switch.o: arch/arm/cortex-m/context_switch.S
	$(CC_arm) $(CFLAGS_arm) -c $< -o $@

src/kernel/%.o: src/kernel/%.c
	$(CC_x86) $(CFLAGS_x86) -c $< -o ${@:.o=_x86.o} 2>/dev/null || true
	$(CC_arm) $(CFLAGS_arm) -c $< -o ${@:.o=_arm.o} 2>/dev/null || true
	mv ${@:.o=_$(shell echo $@ | cut -d_ -f3).o} $@

# QEMU for testing
qemu: x86
	$(QEMU) -kernel build/kernel_x86.elf -serial stdio

clean:
	rm -rf build/*.o build/*.elf build/*.bin
