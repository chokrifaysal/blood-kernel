/*
 * bootloader.c - in-flash OTA via ISO-TP
 * Lives at 0x08000000, jumps to 0x08080000 after CRC
 */

#include "kernel/types.h"
#include "kernel/can.h"
#include "kernel/isotp.h"
#include "kernel/flash.h"
#include "kernel/crc.h"

#define APP_ADDR 0x08080000UL
#define BOOT_TIMEOUT 5000   // ms

void bootloader_main(void) {
    uart_early_init();
    kprintf("BLOOD_BOOT v1.0\r\n");
    
    isotp_init();
    
    u32 start = timer_ticks();
    while (timer_ticks() - start < BOOT_TIMEOUT) {
        isotp_msg_t msg;
        if (isotp_recv(&msg) == 0 && msg.id == 0x7E0) {
            // handle OTA
            flash_erase(APP_ADDR);
            flash_write(APP_ADDR, msg.data, msg.len);
            kprintf("OTA done, jumping\r\n");
            break;
        }
    }
    
    // CRC check app
    if (crc32((u8*)APP_ADDR, 0x10000) == *(u32*)(APP_ADDR + 0x10000)) {
        ((void(*)(void))APP_ADDR)();
    } else {
        kprintf("CRC fail, stay in boot\r\n");
        while (1);
    }
}
