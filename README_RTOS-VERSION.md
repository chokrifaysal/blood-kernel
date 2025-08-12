# BLOOD_KERNEL v1.0 – Automotive RTOS

Minimal real-time microkernel for **SIL-2** ECUs.  
- **< 10 k LOC**, zero-copy IPC, static only  
- **CAN 500 kbit/s**, SD-card logging, watchdog  
- **ARM Cortex-M4 / TI Cortex-R4F**  
- **MISRA-C:2012**, ISO 26262 ASIL-B  

## Quick Start
```bash
# STM32
make arm && openocd -f board/st_nucleo_f4.cfg

# TMS570
make BOARD=tms570 && openocd -f board/ti_tms570ls3137.cfg

# QEMU
make x86 && make qemu
```

## Safety
- *Stack canaries* per task
- *MPU* isolation
- *Watchdog* 2 s refresh
- *CAN RX timeout* reset
- *MISRA* checked via `tools/misra_check.sh`

## License
public domain / 0BSD / AGPL-3.0 / whatevah



