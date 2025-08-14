# AVR128DA48 Support

**8-bit AVR microcontroller @ 20 MHz**

## Features
- **TCA0 Timer**: 1 kHz system tick + PWM on PA0
- **ADC0**: 12-bit, temp sensor, VDD monitoring  
- **EEPROM**: 512 bytes non-volatile storage
- **GPIO**: 5V tolerant on PORTB
- **SPI**: Master mode @ 10 MHz
- **TWI**: I²C slave @ 100 kHz
- **UART**: 115200 baud on USART0

## Memory Map
- **Flash**: 128 kB @ 0x0000
- **SRAM**: 16 kB @ 0x800100  
- **EEPROM**: 512 B @ 0x1400

## Build & Flash
```bash
make ARCH=avr128da
make ARCH=avr128da flash
```

## Pinout
- **PA0**: PWM output (TCA0/WO0)
- **PA4**: SPI MOSI
- **PA5**: SPI MISO  
- **PA6**: SPI SCK
- **PB0-7**: 5V GPIO
- **PC0**: UART TX
- **PC1**: UART RX
- **PD4**: TWI SDA
- **PD5**: TWI SCL

## Demo Task
Reads temp sensor, outputs PWM, stores to EEPROM, blinks LED.
