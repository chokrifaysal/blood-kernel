/*
 * mpu.c - ARM MPU setup for task isolation
 */

#include "kernel/mpu.h"
#include "uart.h"

#define MPU_TYPE    (*(volatile u32*)0xE000ED90)
#define MPU_CTRL    (*(volatile u32*)0xE000ED94)
#define MPU_RNR     (*(volatile u32*)0xE000ED98)
#define MPU_RBAR    (*(volatile u32*)0xE000ED9C)
#define MPU_RASR    (*(volatile u32*)0xE000EDA0)

void mpu_init(void) {
    u8 regions = (MPU_TYPE >> 8) & 0xFF;
    uart_puts("MPU regions: ");
    uart_hex(regions);
    uart_puts("\r\n");
    
    if (regions < 8) {
        uart_puts("MPU too small, skipping\r\n");
        return;
    }
    
    // Region 0: 0x00000000-0x1FFFFFFF Flash (RX)
    mpu_region(0, 0x08000000, MPU_REGION_SIZE_512KB, 0b011, 0);
    
    // Region 1: 0x20000000-0x3FFFFFFF RAM (RW)
    mpu_region(1, 0x20000000, MPU_REGION_SIZE_128KB, 0b011, 0);
    
    // Region 2: 0x40000000-0x5FFFFFFF Peripherals (RW)
    mpu_region(2, 0x40000000, MPU_REGION_SIZE_512MB, 0b011, 1);
    
    // Region 3: Task 0 stack (32 KB, bottom 1 KB no-access)
    mpu_region(3, 0x2001C000 - 32*1024, MPU_REGION_SIZE_32KB, 0b011, 0);
    
    // Region 4: Kernel (0x08000000-0x08080000) no-access for users
    mpu_region(4, 0x08000000, MPU_REGION_SIZE_512KB, 0b000, 0);
    
    mpu_enable();
    uart_puts("MPU enabled\r\n");
}

void mpu_region(u8 region, u32 addr, u8 size, u8 ap, u8 xn) {
    MPU_RNR = region;
    MPU_RBAR = addr & 0xFFFFFFE0;   // 32-byte aligned
    MPU_RASR = (1<<16) | (ap<<24) | (xn<<28) | (size<<1) | 1;
}

void mpu_enable(void) {
    MPU_CTRL = 0x07;   // enable + default map + hardfault
}
