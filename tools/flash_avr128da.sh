#!/bin/sh
# flash_avr128da.sh – AVR-ICE + openocd
avr-objcopy -O ihex build/kernel.elf build/kernel.hex
openocd -f tools/openocd/avr128da.cfg \
        -c "program build/kernel.hex verify reset exit"
