# RP2040 Support

**Dual-core ARM Cortex-M0+ @ 125 MHz with PIO**

## Features
- **PIO State Machines**: 8 programmable I/O processors
- **Hardware Timer**: 64-bit 1 MHz timer with 1 kHz system tick
- **Dual-core**: Cortex-M0+ cores with FIFO communication
- **QSPI Flash**: W25Q16 2MB external flash with XIP
- **PWM**: 8 slices, 16 channels, up to 125 MHz
- **ADC**: 12-bit, 4 channels + temperature sensor
- **RTC**: Real-time clock with alarm
- **Watchdog**: Hardware watchdog timer
- **Hardware Divider**: 32-bit signed/unsigned division
- **Interpolator**: Hardware interpolation engine

## Memory Map
- **Flash**: 2 MB @ 0x10000000 (XIP)
- **SRAM**: 264 kB @ 0x20000000
- **PIO0**: 0x50200000
- **PIO1**: 0x50300000
- **Timer**: 0x40054000
- **SIO**: 0xD0000000

## Build & Flash
```bash
make ARCH=rp2040
make ARCH=rp2040 flash
```

## PIO Programs
- **Blink**: LED blink at 1 Hz
- **UART TX**: Serial transmission at any baud rate
- **SPI**: Master mode with configurable clock
- **WS2812**: RGB LED strip control with precise timing

## PIO API
```c
/* Load and configure PIO program */
u8 offset = pio_load_program(0, program, length);
pio_sm_init(0, 0, offset, &config);
pio_sm_start(0, 0);

/* Send/receive data */
pio_sm_put(0, 0, data);
u32 result = pio_sm_get(0, 0);

/* High-level functions */
pio_load_blink(25);
pio_uart_tx_init(0, 115200);
pio_spi_init(2, 3, 4);
pio_ws2812_init(16);
```

## Multicore API
```c
/* Launch core 1 */
multicore_launch_core1(core1_main);

/* Inter-core communication */
multicore_fifo_push(0x12345678);
u32 data = multicore_fifo_pop();

/* Spinlocks */
multicore_spin_lock_blocking(0);
multicore_spin_lock_release(0);
```

## Timer API
```c
/* System tick (1 kHz) */
u32 ticks = timer_ticks();
timer_delay(1000);

/* Microsecond precision */
u64 us = timer_us();
timer_delay_us(500);
```

## Demo Tasks
- **PIO Blink**: LED on pin 25 via PIO
- **PIO UART**: Serial output on pin 0
- **PIO SPI**: SPI communication on pins 2,3,4
- **WS2812**: RGB LEDs on pin 16
- **PWM**: Breathing LED effect
- **ADC**: Temperature and analog readings
- **Multicore**: Core 0/1 communication
- **Flash**: QSPI read/write/erase operations
- **Timer**: Precision timing tests

## Performance
- **PIO**: Up to 125 MHz I/O operations
- **Flash**: 133 MHz QSPI with XIP
- **Timer**: 1 MHz resolution, 64-bit range
- **Multicore**: Lock-free FIFO communication
- **PWM**: Up to 125 MHz output frequency
