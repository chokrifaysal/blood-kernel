#!/bin/sh
# build_task.sh - compile user ELF for blood_kernel
# usage: ./build_task.sh task.c

set -e

CROSS=arm-none-eabi-
CC=${CROSS}gcc
LD=${CROSS}ld
OBJCOPY=${CROSS}objcopy

TASK_NAME=$(basename "$1" .c)
ELF_OUT="build/${TASK_NAME}.elf"
BIN_OUT="build/${TASK_NAME}.bin"

echo "Building task: $TASK_NAME"

$CC -mcpu=cortex-m4 -mthumb -ffreestanding -nostdlib -nostartfiles \
    -Wl,-T,tasks/task.ld -Wl,-e,task_main -O2 -g -o $ELF_OUT $1

$OBJCOPY -O binary $ELF_OUT $BIN_OUT
echo "ELF -> $ELF_OUT  BIN -> $BIN_OUT"
