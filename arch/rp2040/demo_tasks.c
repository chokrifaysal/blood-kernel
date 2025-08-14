/*
 * demo_tasks.c – RP2040 PIO + timer + multicore demos
 */

#include "kernel/types.h"
#include "pio_driver.h"

extern void timer_delay(u32 ms);
extern u8 multicore_launch_core1(void (*entry)(void));
extern void multicore_fifo_push(u32 data);
extern u32 multicore_fifo_pop(void);
extern u8 multicore_fifo_rvalid(void);
extern void gpio_set_dir(u8 pin, u8 out);
extern void gpio_put(u8 pin, u8 val);
extern void gpio_toggle(u8 pin);

static void pio_blink_task(void) {
    /* PIO blinks LED on pin 25 */
    pio_load_blink(25);
    
    while (1) {
        timer_delay(1000);
    }
}

static void pio_uart_task(void) {
    /* PIO UART TX on pin 0 at 115200 baud */
    pio_uart_tx_init(0, 115200);
    
    const char* msg = "RP2040 PIO UART\r\n";
    u8 i = 0;
    
    while (1) {
        pio_uart_tx_byte(msg[i]);
        i++;
        if (msg[i] == 0) i = 0;
        timer_delay(100);
    }
}

static void pio_spi_task(void) {
    /* PIO SPI on pins 2,3,4 (CLK,MOSI,MISO) */
    pio_spi_init(2, 3, 4);
    
    u8 test_data = 0xAA;
    
    while (1) {
        u8 response = pio_spi_xfer(test_data);
        test_data = response + 1;
        timer_delay(500);
    }
}

static void pio_ws2812_task(void) {
    /* WS2812 RGB LEDs on pin 16 */
    pio_ws2812_init(16);
    
    u32 colors[] = {
        0xFF0000, /* Red */
        0x00FF00, /* Green */
        0x0000FF, /* Blue */
        0xFFFF00, /* Yellow */
        0xFF00FF, /* Magenta */
        0x00FFFF, /* Cyan */
        0xFFFFFF, /* White */
        0x000000  /* Off */
    };
    
    u8 color_idx = 0;
    
    while (1) {
        pio_ws2812_put_pixel(colors[color_idx]);
        color_idx = (color_idx + 1) % 8;
        timer_delay(250);
    }
}

static void pwm_task(void) {
    /* PWM on slice 0, channel A (pin 0) */
    pwm_init(0, 0, 1000); /* 1 kHz */
    
    u16 duty = 0;
    s8 direction = 1;
    
    while (1) {
        pwm_set_duty(0, 0, duty);
        
        duty += direction * 1000;
        if (duty >= 65000) direction = -1;
        if (duty <= 1000) direction = 1;
        
        timer_delay(10);
    }
}

static void adc_task(void) {
    adc_init();
    
    while (1) {
        u16 temp = adc_read_temp();
        u16 ch0 = adc_read(0);
        u16 ch1 = adc_read(1);
        
        /* Temperature calculation: temp_c = 27 - (temp - 0.706) / 0.001721 */
        s16 temp_c = 27 - ((temp * 3300 / 4096) - 706) / 2;
        
        (void)temp_c;
        (void)ch0;
        (void)ch1;
        
        timer_delay(1000);
    }
}

static void watchdog_task(void) {
    watchdog_init(5000); /* 5 second timeout */
    
    while (1) {
        watchdog_feed();
        timer_delay(1000);
    }
}

static void rtc_task(void) {
    rtc_init();
    
    while (1) {
        u32 time = rtc_get_time();
        u32 date = rtc_get_date();
        
        u8 sec = time & 0x3F;
        u8 min = (time >> 8) & 0x3F;
        u8 hour = (time >> 16) & 0x1F;
        
        u8 day = date & 0x1F;
        u8 month = (date >> 8) & 0x0F;
        u16 year = (date >> 12) & 0xFFF;
        
        (void)sec; (void)min; (void)hour;
        (void)day; (void)month; (void)year;
        
        timer_delay(1000);
    }
}

static void flash_task(void) {
    flash_init();
    
    u32 flash_id = flash_read_id();
    (void)flash_id;
    
    /* Test flash operations */
    u8 test_data[256];
    u8 read_data[256];
    
    for (u16 i = 0; i < 256; i++) {
        test_data[i] = i & 0xFF;
    }
    
    while (1) {
        /* Erase sector at 1MB offset */
        flash_sector_erase(0x100000);
        
        /* Write test data */
        flash_write(0x100000, test_data, 256);
        
        /* Read back and verify */
        flash_read(0x100000, read_data, 256);
        
        u8 verify_ok = 1;
        for (u16 i = 0; i < 256; i++) {
            if (test_data[i] != read_data[i]) {
                verify_ok = 0;
                break;
            }
        }
        
        (void)verify_ok;
        timer_delay(10000);
    }
}

static void core1_task(void) {
    /* Core 1 main task */
    gpio_set_dir(24, 1); /* LED output */
    
    while (1) {
        /* Check for messages from core 0 */
        if (multicore_fifo_rvalid()) {
            u32 msg = multicore_fifo_pop();
            
            if (msg == 0x12345678) {
                /* Blink LED faster */
                for (u8 i = 0; i < 10; i++) {
                    gpio_toggle(24);
                    timer_delay(50);
                }
                
                /* Send response */
                multicore_fifo_push(0x87654321);
            }
        }
        
        /* Normal LED blink */
        gpio_toggle(24);
        timer_delay(500);
    }
}

static void multicore_demo_task(void) {
    /* Launch core 1 */
    if (multicore_launch_core1(core1_task)) {
        /* Core 1 launched successfully */
        while (1) {
            /* Send message to core 1 */
            multicore_fifo_push(0x12345678);
            
            /* Wait for response */
            u32 response = multicore_fifo_pop();
            if (response == 0x87654321) {
                /* Core 1 responded correctly */
            }
            
            timer_delay(2000);
        }
    }
}

static void timer_precision_task(void) {
    u64 start_us, end_us;
    
    while (1) {
        /* Test microsecond precision */
        start_us = timer_us();
        timer_delay_us(1000); /* 1ms delay */
        end_us = timer_us();
        
        u32 actual_delay = end_us - start_us;
        (void)actual_delay; /* Should be ~1000 us */
        
        timer_delay(1000);
    }
}

void rp2040_demo_init(void) {
    /* Called from kernel_main for RP2040 */
    extern void task_create(void (*func)(void), u8 prio, u16 stack_size);
    
    /* Initialize peripherals */
    pio_init();
    
    /* Create demo tasks */
    task_create(pio_blink_task, 1, 256);
    task_create(pio_uart_task, 2, 256);
    task_create(pio_spi_task, 3, 256);
    task_create(pio_ws2812_task, 4, 256);
    task_create(pwm_task, 5, 256);
    task_create(adc_task, 6, 256);
    task_create(watchdog_task, 7, 256);
    task_create(rtc_task, 8, 256);
    task_create(flash_task, 9, 512);
    task_create(multicore_demo_task, 10, 256);
    task_create(timer_precision_task, 11, 256);
}
