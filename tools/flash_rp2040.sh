#!/bin/sh
# flash_rp2040.sh – flash via picoprobe
openocd -f interface/picoprobe.cfg -f target/rp2040.cfg \
        -c "program build/kernel.elf verify reset exit"
