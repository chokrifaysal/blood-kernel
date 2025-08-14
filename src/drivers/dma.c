/*
 * dma.c – x86 8237A DMA controller
 */

#include "kernel/types.h"

/* DMA controller registers */
#define DMA1_STATUS       0x08
#define DMA1_COMMAND      0x08
#define DMA1_REQUEST      0x09
#define DMA1_MASK         0x0A
#define DMA1_MODE         0x0B
#define DMA1_CLEAR_FF     0x0C
#define DMA1_TEMP         0x0D
#define DMA1_MASTER_CLEAR 0x0D
#define DMA1_CLEAR_MASK   0x0E
#define DMA1_WRITE_MASK   0x0F

#define DMA2_STATUS       0xD0
#define DMA2_COMMAND      0xD0
#define DMA2_REQUEST      0xD2
#define DMA2_MASK         0xD4
#define DMA2_MODE         0xD6
#define DMA2_CLEAR_FF     0xD8
#define DMA2_TEMP         0xDA
#define DMA2_MASTER_CLEAR 0xDA
#define DMA2_CLEAR_MASK   0xDC
#define DMA2_WRITE_MASK   0xDE

/* DMA channel registers */
static const u16 dma_addr_ports[] = {0x00, 0x02, 0x04, 0x06, 0xC0, 0xC4, 0xC8, 0xCC};
static const u16 dma_count_ports[] = {0x01, 0x03, 0x05, 0x07, 0xC2, 0xC6, 0xCA, 0xCE};
static const u16 dma_page_ports[] = {0x87, 0x83, 0x81, 0x82, 0x8F, 0x8B, 0x89, 0x8A};

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

typedef struct {
    u32 address;
    u16 count;
    u8 mode;
    u8 in_use;
} dma_channel_t;

static dma_channel_t dma_channels[8];

static inline void outb(u16 port, u8 val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline u8 inb(u16 port) {
    u8 val;
    __asm__ volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

void dma_init(void) {
    /* Reset both DMA controllers */
    outb(DMA1_MASTER_CLEAR, 0);
    outb(DMA2_MASTER_CLEAR, 0);
    
    /* Clear all channels */
    for (u8 i = 0; i < 8; i++) {
        dma_channels[i].address = 0;
        dma_channels[i].count = 0;
        dma_channels[i].mode = 0;
        dma_channels[i].in_use = 0;
    }
    
    /* Mask all channels */
    outb(DMA1_WRITE_MASK, 0x0F);
    outb(DMA2_WRITE_MASK, 0x0F);
    
    /* Set command registers */
    outb(DMA1_COMMAND, 0x00); /* Normal operation */
    outb(DMA2_COMMAND, 0x00);
}

u8 dma_allocate_channel(u8 channel) {
    if (channel >= 8 || channel == 4) return 0; /* Channel 4 is cascade */
    
    if (dma_channels[channel].in_use) return 0;
    
    dma_channels[channel].in_use = 1;
    return 1;
}

void dma_free_channel(u8 channel) {
    if (channel >= 8) return;
    
    dma_mask_channel(channel);
    dma_channels[channel].in_use = 0;
}

void dma_setup_channel(u8 channel, u32 address, u16 count, u8 mode) {
    if (channel >= 8 || channel == 4) return;
    
    dma_channels[channel].address = address;
    dma_channels[channel].count = count;
    dma_channels[channel].mode = mode;
    
    /* Mask channel */
    dma_mask_channel(channel);
    
    if (channel < 4) {
        /* 8-bit DMA controller */
        outb(DMA1_CLEAR_FF, 0);
        
        /* Set mode */
        outb(DMA1_MODE, (channel & 3) | mode);
        
        /* Set address */
        outb(dma_addr_ports[channel], address & 0xFF);
        outb(dma_addr_ports[channel], (address >> 8) & 0xFF);
        
        /* Set page */
        outb(dma_page_ports[channel], (address >> 16) & 0xFF);
        
        /* Set count */
        count--;
        outb(dma_count_ports[channel], count & 0xFF);
        outb(dma_count_ports[channel], (count >> 8) & 0xFF);
    } else {
        /* 16-bit DMA controller */
        outb(DMA2_CLEAR_FF, 0);
        
        /* Set mode */
        outb(DMA2_MODE, (channel & 3) | mode);
        
        /* Address is in words for 16-bit DMA */
        address >>= 1;
        count >>= 1;
        
        /* Set address */
        outb(dma_addr_ports[channel], address & 0xFF);
        outb(dma_addr_ports[channel], (address >> 8) & 0xFF);
        
        /* Set page */
        outb(dma_page_ports[channel], (address >> 16) & 0xFF);
        
        /* Set count */
        count--;
        outb(dma_count_ports[channel], count & 0xFF);
        outb(dma_count_ports[channel], (count >> 8) & 0xFF);
    }
}

void dma_start_transfer(u8 channel) {
    if (channel >= 8 || channel == 4) return;
    
    dma_unmask_channel(channel);
}

void dma_mask_channel(u8 channel) {
    if (channel >= 8) return;
    
    if (channel < 4) {
        outb(DMA1_MASK, 0x04 | (channel & 3));
    } else {
        outb(DMA2_MASK, 0x04 | (channel & 3));
    }
}

void dma_unmask_channel(u8 channel) {
    if (channel >= 8) return;
    
    if (channel < 4) {
        outb(DMA1_MASK, channel & 3);
    } else {
        outb(DMA2_MASK, channel & 3);
    }
}

u8 dma_is_transfer_complete(u8 channel) {
    if (channel >= 8) return 1;
    
    u8 status;
    if (channel < 4) {
        status = inb(DMA1_STATUS);
        return (status & (1 << channel)) != 0;
    } else {
        status = inb(DMA2_STATUS);
        return (status & (1 << (channel - 4))) != 0;
    }
}

u16 dma_get_remaining_count(u8 channel) {
    if (channel >= 8 || channel == 4) return 0;
    
    u16 count;
    
    if (channel < 4) {
        outb(DMA1_CLEAR_FF, 0);
        count = inb(dma_count_ports[channel]);
        count |= inb(dma_count_ports[channel]) << 8;
    } else {
        outb(DMA2_CLEAR_FF, 0);
        count = inb(dma_count_ports[channel]);
        count |= inb(dma_count_ports[channel]) << 8;
        count <<= 1; /* Convert from words to bytes */
    }
    
    return count + 1;
}

void dma_reset_controller(u8 controller) {
    if (controller == 0) {
        outb(DMA1_MASTER_CLEAR, 0);
        outb(DMA1_COMMAND, 0x00);
        outb(DMA1_WRITE_MASK, 0x0F);
    } else if (controller == 1) {
        outb(DMA2_MASTER_CLEAR, 0);
        outb(DMA2_COMMAND, 0x00);
        outb(DMA2_WRITE_MASK, 0x0F);
    }
}

u8 dma_get_status(u8 controller) {
    if (controller == 0) {
        return inb(DMA1_STATUS);
    } else if (controller == 1) {
        return inb(DMA2_STATUS);
    }
    return 0;
}

void dma_set_mode(u8 channel, u8 mode) {
    if (channel >= 8 || channel == 4) return;
    
    dma_channels[channel].mode = mode;
    
    if (channel < 4) {
        outb(DMA1_MODE, (channel & 3) | mode);
    } else {
        outb(DMA2_MODE, (channel & 3) | mode);
    }
}

u8 dma_is_channel_available(u8 channel) {
    if (channel >= 8 || channel == 4) return 0;
    return !dma_channels[channel].in_use;
}

void dma_wait_for_completion(u8 channel) {
    if (channel >= 8) return;
    
    while (!dma_is_transfer_complete(channel)) {
        __asm__ volatile("pause");
    }
}

/* High-level transfer functions */
u8 dma_memory_to_device(u8 channel, u32 memory_addr, u16 size) {
    if (!dma_allocate_channel(channel)) return 0;
    
    dma_setup_channel(channel, memory_addr, size, 
                     DMA_MODE_SINGLE | DMA_MODE_READ);
    dma_start_transfer(channel);
    
    return 1;
}

u8 dma_device_to_memory(u8 channel, u32 memory_addr, u16 size) {
    if (!dma_allocate_channel(channel)) return 0;
    
    dma_setup_channel(channel, memory_addr, size, 
                     DMA_MODE_SINGLE | DMA_MODE_WRITE);
    dma_start_transfer(channel);
    
    return 1;
}

u8 dma_verify_transfer(u8 channel, u32 memory_addr, u16 size) {
    if (!dma_allocate_channel(channel)) return 0;
    
    dma_setup_channel(channel, memory_addr, size, 
                     DMA_MODE_SINGLE | DMA_MODE_VERIFY);
    dma_start_transfer(channel);
    
    return 1;
}
