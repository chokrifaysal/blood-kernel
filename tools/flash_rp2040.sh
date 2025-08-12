#!/bin/sh
# flash_rp2040.sh – UF2less flash via picoprobe
openocd -f interface/picoprobe.cfg -f target/rp2040.cfg \
        -c "program build/kernel.elf reset exit"
chmod +x tools/flash_rp2040.sh
