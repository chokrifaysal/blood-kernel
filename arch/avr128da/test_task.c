/*
 * test_task.c – AVR128DA48 demo task
 */

#include "kernel/types.h"
#include "avr128da.h"

static void sensor_task(void) {
    u16 temp, vdd;
    u8 cnt = 0;
    
    adc_init();
    pwm_init();
    
    while (1) {
        temp = adc_temp();
        vdd = adc_vdd();
        
        /* PWM duty based on temp */
        pwm_set(temp >> 4);
        
        /* Store to EEPROM every 10 readings */
        if (++cnt >= 10) {
            eeprom_write(0, temp & 0xFF);
            eeprom_write(1, temp >> 8);
            cnt = 0;
        }
        
        /* Blink GPIO */
        gpio_toggle(7);
        
        timer_delay(100);
    }
}

void avr_demo_init(void) {
    /* Called from kernel_main for AVR128DA */
    task_create(sensor_task, 0, 128);
}
