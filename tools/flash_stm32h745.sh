#!/bin/sh
# flash_stm32h745.sh – dual-core via J-Link
openocd -f interface/jlink.cfg -c "transport select swd" \
        -f target/stm32h745.cfg \
        -c "adapter_khz 4000" \
        -c "program build/kernel_cm7.elf verify reset exit" \
        -c "program build/kernel_cm4.elf verify reset exit"
chmod +x tools/flash_stm32h745.sh
