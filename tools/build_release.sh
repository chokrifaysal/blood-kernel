#!/bin/sh
# build_release.sh – produce final images
set -e

echo "=== BLOOD_KERNEL v1.1 RELEASE BUILD ==="

mkdir -p release

# kernel
make BOARD=tms570 clean && make BOARD=tms570
cp build/kernel.elf release/kernel_tms570.elf
cp build/kernel.bin release/kernel_tms570.bin

# bootloader
make -C boot clean && make -C boot
cp boot/bootloader.bin release/bootloader.bin

# safety tests
make -C tests clean && make -C tests
cp build/safety_test.elf release/safety_test.elf

# checksums
sha256sum release/* > release/SHA256SUMS

echo "Release ready in ./release/"
