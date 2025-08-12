#!/bin/sh
# flash_esp32s3.sh – esptool.py via USB
esptool.py --chip esp32s3 --port /dev/ttyUSB0 \
           --before default_reset --after hard_reset \
           write_flash 0x0000 build/kernel.elf
chmod +x tools/flash_esp32s3.sh
