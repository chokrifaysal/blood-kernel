#!/bin/sh
# flash_gd32vf103.sh – J-Link RV32
openocd -f interface/jlink.cfg -c "transport select jtag" \
        -f target/gd32vf103.cfg \
        -c "adapter_khz 4000" \
        -c "program build/kernel.elf verify reset exit"
chmod +x tools/flash_gd32vf103.sh
