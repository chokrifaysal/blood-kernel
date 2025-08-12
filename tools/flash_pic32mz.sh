#!/bin/sh
# flash_pic32mz.sh – PICkit4 via openocd
openocd -f tools/interfaces/pic32mz.cfg \
        -c "program build/kernel.elf verify reset exit"
chmod +x tools/flash_pic32mz.sh
