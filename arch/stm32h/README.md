# STM32H745 Support

**Dual-core ARM Cortex-M7 @ 480 MHz + M4 @ 240 MHz**

## Features
- **CAN-FD**: 1 Mbit/s nominal, 8 Mbit/s data rate
- **Ethernet PHY**: LAN8742A with auto-negotiation
- **QSPI Flash**: W25Q128 16MB external flash
- **USB HS OTG**: Device mode with HS support
- **Dual-core IPC**: Mailbox between CM7 and CM4
- **UART3**: 115200 baud shared console

## Memory Map
- **Flash**: 2 MB @ 0x08000000 (CM7), 0x08100000 (CM4)
- **SRAM**: 1 MB total (multiple regions)
- **DTCM**: 128 kB @ 0x20000000 (CM7 only)
- **ITCM**: 64 kB @ 0x00000000 (CM7 only)

## Build & Flash
```bash
make ARCH=stm32h
make ARCH=stm32h flash
```

## Pinout
- **CAN-FD**: PD0 (RX), PD1 (TX)
- **Ethernet**: PA1,PA2,PA7,PB13,PC1,PC4,PC5,PG11,PG13
- **QSPI**: PB2,PB6,PD11,PD12,PD13,PE2
- **USB HS**: PB14 (D-), PB15 (D+)
- **UART3**: PD8 (TX), PD9 (RX)

## Demo Tasks
- **CAN-FD**: Sends/receives FD frames with BRS
- **Ethernet**: PHY status monitoring
- **QSPI**: Flash read/write/erase operations
- **USB**: Device enumeration and configuration

## Performance
- **CAN-FD**: Up to 8 Mbit/s data phase
- **Ethernet**: 100 Mbit/s full duplex
- **QSPI**: Up to 133 MHz quad SPI
- **USB**: High-speed 480 Mbit/s
