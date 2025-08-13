#!/bin/sh
# flash_ra6m5.sh – CMSIS-DAP via openocd
openocd -f tools/interfaces/ra6m5.cfg \
        -c "program build/kernel.elf verify reset exit"
chmod +x tools/flash_ra6m5.sh
