# BLOOD_KERNEL Automotive Safety Manual  
v1.0 – 2025-08-13

## System Overview  
Minimal RTOS for **SIL-2** automotive ECUs.  
- **MCU**: STM32F407 / TMS570LS3137  
- **OS**: Static only, no heap, no dynamic linking  
- **Standards**: MISRA-C:2012 (checked), ISO 26262 ASIL-B  

## Safety Functions  
| Function            | ASIL | Notes |
|---------------------|------|-------|
| Task watchdog       | B    | 2 s IWDG, refreshed in idle |
| Stack overflow      | B    | Canary per task, yield check |
| CAN bus monitoring  | B    | RX timeout 100 ms, bus-off ISR |
| ECC SD-card         | B    | CRC per SD block, ELM logs |
| MPU protection      | B    | Task stacks read-only, flash RX |

## Failure Modes & Mitigations  
| Failure                | Mitigation |
|------------------------|------------|
| Stack smash            | Canary + MPU region lock |
| CAN bus loss           | ISR watchdog reset |
| SD-card corruption     | CRC + redundant log blocks |
| Watchdog starvation    | Idle task refresh only |
| Task starvation        | Round-robin + preemption |

## Build Flags  
```bash
CFLAGS += -Werror -Wall -Wextra -Wshadow -Wconversion
CFLAGS += -std=c11 -ffreestanding -nostdlib -fno-common -fno-builtin
```

## Test Matrix
| Test Case           | Tool         | Pass Criteria |
|---------------------|--------------|---------------|
| Stack overflow      | qemu + gdb   | HardFault + log |
| CAN RX timeout      | can-utils    | Reset within 2 s |
| SD write 10k blocks | stm32flash   | CRC 100 % |
| Idle watchdog       | logic probe  | WDI pulse every 1 s |

## Flash Layout
- 0x08000000 – 0x0807FFFF : kernel (512 kB)
- 0x08080000 – 0x080FFFFF : tasks (512 kB)
- 0x20000000 – 0x2001FFFF : RAM (128 kB)
- 0x20020000 – 0x2002FFFF : log buffer (64 kB)

