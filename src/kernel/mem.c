/*
 * mem.c - parse multiboot memory map, read ARM SCB
 */

#include "kernel/mem.h"
#include "kernel/uart.h"
#include "kernel/types.h"

#ifdef __x86_64__            // actually i686

struct multiboot_mmap {
    u32 size;
    u64 base_addr;
    u64 length;
    u32 type;
} PACKED;

void detect_memory_x86(u32 mbi_addr) {
    if (!mbi_addr) {
        uart_puts("no multiboot info\r\n");
        return;
    }
    
    u32* mbi = (u32*)mbi_addr;
    if (!(mbi[0] & (1<<6))) {   // bit6 = memory map
        uart_puts("no memory map\r\n");
        return;
    }
    
    struct multiboot_mmap* mmap = (struct multiboot_mmap*)mbi[11];
    u32 mmap_end = mbi[11] + mbi[10];
    
    uart_puts("Memory map:\r\n");
    while ((u32)mmap < mmap_end) {
        if (mmap->type == 1) {   // available RAM
            uart_hex((u32)mmap->base_addr);
            uart_puts("-");
            uart_hex((u32)(mmap->base_addr + mmap->length));
            uart_puts("  ");
            uart_hex((u32)mmap->length);
            uart_puts(" bytes\r\n");
        }
        mmap = (struct multiboot_mmap*)((u32)mmap + mmap->size + 4);
    }
}

#elif defined(__arm__)       // ARM Cortex-M4

#define SCB_BASE 0xE000E000
#define SCB_CCIDR (*(volatile u32*)(SCB_BASE + 0xD20))   // cache size
#define SCB_CSSELR (*(volatile u32*)(SCB_BASE + 0xD40))  // cache select

void detect_memory_arm(void) {
    // STM32F4: fixed 128K SRAM, 512K flash
    
    uart_puts("ARM memory:\r\n");
    uart_puts("FLASH: 0x08000000-0x08080000 (512K)\r\n");
    uart_puts("RAM:   0x20000000-0x20020000 (128K)\r\n");
    
    // TODO: read MPU for actual RAM size
}

#endif
