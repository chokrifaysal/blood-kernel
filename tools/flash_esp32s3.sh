#!/bin/sh
# flash_esp32s3.sh
PORT=${1:-/dev/ttyUSB0}
BAUD=${2:-921600}
ELF=build/kernel.elf

echo "Flashing ESP32-S3 via $PORT @ $BAUD"
python3 tools/esptool.py \
    --chip esp32s3 \
    --port "$PORT" \
    --baud "$BAUD" \
    --before default_reset \
    --after hard_reset \
    write_flash \
    --flash_mode dio \
    --flash_freq 40m \
    --flash_size detect \
    0x0000 "$ELF"
