#!/bin/sh
# flash_rt1170.sh – J-Link SWD 4 MHz
openocd -f interface/jlink.cfg -c "transport select swd" \
        -f target/rt1170.cfg \
        -c "adapter_khz 4000" \
        -c "program build/kernel.elf verify reset exit"
chmod +x tools/flash_rt1170.sh
