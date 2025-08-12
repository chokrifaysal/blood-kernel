#!/bin/sh
# flash_avr128da.sh – AVR-ICE + openocd
openocd -f tools/interfaces/avr128da.cfg \
        -c "program build/kernel.hex verify reset exit"
chmod +x tools/flash_avr128da.sh
