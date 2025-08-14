/*
 * dma.h – x86 8237A DMA controller
 */

#ifndef DMA_H
#define DMA_H

#include "kernel/types.h"

/* DMA modes */
#define DMA_MODE_DEMAND   0x00
#define DMA_MODE_SINGLE   0x40
#define DMA_MODE_BLOCK    0x80
#define DMA_MODE_CASCADE  0xC0

#define DMA_MODE_VERIFY   0x00
#define DMA_MODE_WRITE    0x04
#define DMA_MODE_READ     0x08

#define DMA_MODE_AUTOINIT 0x10
#define DMA_MODE_DECREMENT 0x20

/* Core functions */
void dma_init(void);

/* Channel management */
u8 dma_allocate_channel(u8 channel);
void dma_free_channel(u8 channel);
u8 dma_is_channel_available(u8 channel);

/* Transfer setup */
void dma_setup_channel(u8 channel, u32 address, u16 count, u8 mode);
void dma_start_transfer(u8 channel);
void dma_mask_channel(u8 channel);
void dma_unmask_channel(u8 channel);

/* Status functions */
u8 dma_is_transfer_complete(u8 channel);
u16 dma_get_remaining_count(u8 channel);
void dma_wait_for_completion(u8 channel);

/* Control functions */
void dma_reset_controller(u8 controller);
u8 dma_get_status(u8 controller);
void dma_set_mode(u8 channel, u8 mode);

/* High-level transfer functions */
u8 dma_memory_to_device(u8 channel, u32 memory_addr, u16 size);
u8 dma_device_to_memory(u8 channel, u32 memory_addr, u16 size);
u8 dma_verify_transfer(u8 channel, u32 memory_addr, u16 size);

#endif
