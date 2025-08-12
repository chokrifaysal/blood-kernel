/*
 * mpu.h - ARM Cortex-M4 MPU driver
 */

#ifndef _BLOOD_MPU_H
#define _BLOOD_MPU_H

#include "kernel/types.h"

#define MPU_REGION_SIZE_32B   0x04
#define MPU_REGION_SIZE_64B   0x05
#define MPU_REGION_SIZE_128B  0x06
#define MPU_REGION_SIZE_256B  0x07
#define MPU_REGION_SIZE_512B  0x08
#define MPU_REGION_SIZE_1KB   0x09
#define MPU_REGION_SIZE_2KB   0x0A
#define MPU_REGION_SIZE_4KB   0x0B
#define MPU_REGION_SIZE_8KB   0x0C
#define MPU_REGION_SIZE_16KB  0x0D
#define MPU_REGION_SIZE_32KB  0x0E
#define MPU_REGION_SIZE_64KB  0x0F
#define MPU_REGION_SIZE_128KB 0x10
#define MPU_REGION_SIZE_256KB 0x11
#define MPU_REGION_SIZE_512KB 0x12
#define MPU_REGION_SIZE_1MB   0x13
#define MPU_REGION_SIZE_2MB   0x14
#define MPU_REGION_SIZE_4MB   0x15
#define MPU_REGION_SIZE_8MB   0x16
#define MPU_REGION_SIZE_16MB  0x17
#define MPU_REGION_SIZE_32MB  0x18
#define MPU_REGION_SIZE_64MB  0x19
#define MPU_REGION_SIZE_128MB 0x1A
#define MPU_REGION_SIZE_256MB 0x1B
#define MPU_REGION_SIZE_512MB 0x1C
#define MPU_REGION_SIZE_1GB   0x1D
#define MPU_REGION_SIZE_2GB   0x1E
#define MPU_REGION_SIZE_4GB   0x1F

void mpu_init(void);
void mpu_region(u8 region, u32 addr, u8 size, u8 ap, u8 xn);
void mpu_enable(void);

#endif
