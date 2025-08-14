/*
 * rtl8139.c – Realtek RTL8139 Fast Ethernet controller
 */

#include "kernel/types.h"

/* RTL8139 registers */
#define RTL8139_IDR0        0x00  /* MAC address */
#define RTL8139_IDR1        0x01
#define RTL8139_IDR2        0x02
#define RTL8139_IDR3        0x03
#define RTL8139_IDR4        0x04
#define RTL8139_IDR5        0x05
#define RTL8139_MAR0        0x08  /* Multicast registers */
#define RTL8139_MAR1        0x09
#define RTL8139_MAR2        0x0A
#define RTL8139_MAR3        0x0B
#define RTL8139_MAR4        0x0C
#define RTL8139_MAR5        0x0D
#define RTL8139_MAR6        0x0E
#define RTL8139_MAR7        0x0F
#define RTL8139_TSD0        0x10  /* Transmit Status of Descriptor 0 */
#define RTL8139_TSD1        0x14
#define RTL8139_TSD2        0x18
#define RTL8139_TSD3        0x1C
#define RTL8139_TSAD0       0x20  /* Transmit Start Address of Descriptor 0 */
#define RTL8139_TSAD1       0x24
#define RTL8139_TSAD2       0x28
#define RTL8139_TSAD3       0x2C
#define RTL8139_RBSTART     0x30  /* Receive Buffer Start Address */
#define RTL8139_ERBCR       0x34  /* Early Receive Byte Count Register */
#define RTL8139_ERSR        0x36  /* Early Receive Status Register */
#define RTL8139_CR          0x37  /* Command Register */
#define RTL8139_CAPR        0x38  /* Current Address of Packet Read */
#define RTL8139_CBR         0x3A  /* Current Buffer Address */
#define RTL8139_IMR         0x3C  /* Interrupt Mask Register */
#define RTL8139_ISR         0x3E  /* Interrupt Status Register */
#define RTL8139_TCR         0x40  /* Transmit Configuration Register */
#define RTL8139_RCR         0x44  /* Receive Configuration Register */
#define RTL8139_TCTR        0x48  /* Timer Count Register */
#define RTL8139_MPC         0x4C  /* Missed Packet Counter */
#define RTL8139_9346CR      0x50  /* 93C46 Command Register */
#define RTL8139_CONFIG0     0x51  /* Configuration Register 0 */
#define RTL8139_CONFIG1     0x52  /* Configuration Register 1 */
#define RTL8139_BMCR        0x62  /* Basic Mode Control Register */
#define RTL8139_BMSR        0x64  /* Basic Mode Status Register */

/* Command Register bits */
#define RTL8139_CR_RST      0x10  /* Reset */
#define RTL8139_CR_RE       0x08  /* Receiver Enable */
#define RTL8139_CR_TE       0x04  /* Transmitter Enable */
#define RTL8139_CR_BUFE     0x01  /* Buffer Empty */

/* Interrupt Status/Mask Register bits */
#define RTL8139_INT_SERR    0x8000  /* System Error */
#define RTL8139_INT_TIMEOUT 0x4000  /* Time Out */
#define RTL8139_INT_LENCHG  0x2000  /* Cable Length Change */
#define RTL8139_INT_FOVW    0x0040  /* Rx FIFO Overflow */
#define RTL8139_INT_PUN     0x0020  /* Packet Underrun */
#define RTL8139_INT_RXOVW   0x0010  /* Rx Buffer Overflow */
#define RTL8139_INT_TER     0x0008  /* Transmit Error */
#define RTL8139_INT_TOK     0x0004  /* Transmit OK */
#define RTL8139_INT_RER     0x0002  /* Receive Error */
#define RTL8139_INT_ROK     0x0001  /* Receive OK */

/* Transmit Configuration Register bits */
#define RTL8139_TCR_IFG     0x03000000  /* Interframe Gap */
#define RTL8139_TCR_MXDMA   0x00700000  /* Max DMA Burst Size */
#define RTL8139_TCR_CRC     0x00010000  /* CRC Append */
#define RTL8139_TCR_LBK     0x00060000  /* Loopback Test */

/* Receive Configuration Register bits */
#define RTL8139_RCR_ERTH    0xF0000000  /* Early Rx Threshold */
#define RTL8139_RCR_MER     0x00800000  /* Multiple Early Interrupt */
#define RTL8139_RCR_RXFTH   0x00E00000  /* Rx FIFO Threshold */
#define RTL8139_RCR_RBLEN   0x00001800  /* Rx Buffer Length */
#define RTL8139_RCR_MXDMA   0x00000700  /* Max DMA Burst Size */
#define RTL8139_RCR_WRAP    0x00000080  /* Wrap */
#define RTL8139_RCR_AER     0x00000020  /* Accept Error Packets */
#define RTL8139_RCR_AR      0x00000010  /* Accept Runt Packets */
#define RTL8139_RCR_AB      0x00000008  /* Accept Broadcast Packets */
#define RTL8139_RCR_AM      0x00000004  /* Accept Multicast Packets */
#define RTL8139_RCR_APM     0x00000002  /* Accept Physical Match Packets */
#define RTL8139_RCR_AAP     0x00000001  /* Accept All Packets */

/* Transmit Status Descriptor bits */
#define RTL8139_TSD_CRS     0x80000000  /* Carrier Sense Lost */
#define RTL8139_TSD_TABT    0x40000000  /* Transmit Abort */
#define RTL8139_TSD_OWC     0x20000000  /* Out of Window Collision */
#define RTL8139_TSD_CDH     0x10000000  /* CD Heart Beat */
#define RTL8139_TSD_NCC     0x0F000000  /* Number of Collision Count */
#define RTL8139_TSD_ERTXTH  0x001F0000  /* Early Tx Threshold */
#define RTL8139_TSD_TOK     0x00008000  /* Transmit OK */
#define RTL8139_TSD_TUN     0x00004000  /* Transmit FIFO Underrun */
#define RTL8139_TSD_OWN     0x00002000  /* Owner */
#define RTL8139_TSD_SIZE    0x00001FFF  /* Descriptor Size */

#define RX_BUFFER_SIZE      8192
#define TX_BUFFER_SIZE      1536

typedef struct {
    u16 io_base;
    u8 irq;
    u8 mac_addr[6];
    u8* rx_buffer;
    u8* tx_buffers[4];
    u16 rx_offset;
    u8 tx_current;
    u8 initialized;
} rtl8139_device_t;

static rtl8139_device_t rtl8139_dev;

static inline void outb(u16 port, u8 val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline u8 inb(u16 port) {
    u8 val;
    __asm__ volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static inline void outw(u16 port, u16 val) {
    __asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline u16 inw(u16 port) {
    u16 val;
    __asm__ volatile("inw %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static inline void outl(u16 port, u32 val) {
    __asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline u32 inl(u16 port) {
    u32 val;
    __asm__ volatile("inl %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

u8 rtl8139_init(u16 io_base, u8 irq) {
    rtl8139_dev.io_base = io_base;
    rtl8139_dev.irq = irq;
    
    /* Power on the device */
    outb(io_base + RTL8139_CONFIG1, 0x00);
    
    /* Software reset */
    outb(io_base + RTL8139_CR, RTL8139_CR_RST);
    
    /* Wait for reset to complete */
    u32 timeout = 1000000;
    while (timeout-- && (inb(io_base + RTL8139_CR) & RTL8139_CR_RST));
    
    if (inb(io_base + RTL8139_CR) & RTL8139_CR_RST) {
        return 0; /* Reset failed */
    }
    
    /* Read MAC address */
    for (u8 i = 0; i < 6; i++) {
        rtl8139_dev.mac_addr[i] = inb(io_base + RTL8139_IDR0 + i);
    }
    
    /* Allocate receive buffer */
    extern void* paging_alloc_pages(u32 count);
    rtl8139_dev.rx_buffer = (u8*)paging_alloc_pages(2); /* 8KB */
    if (!rtl8139_dev.rx_buffer) return 0;
    
    /* Allocate transmit buffers */
    for (u8 i = 0; i < 4; i++) {
        rtl8139_dev.tx_buffers[i] = (u8*)paging_alloc_pages(1); /* 4KB each */
        if (!rtl8139_dev.tx_buffers[i]) return 0;
    }
    
    /* Set receive buffer */
    outl(io_base + RTL8139_RBSTART, (u32)rtl8139_dev.rx_buffer);
    
    /* Set transmit buffer addresses */
    outl(io_base + RTL8139_TSAD0, (u32)rtl8139_dev.tx_buffers[0]);
    outl(io_base + RTL8139_TSAD1, (u32)rtl8139_dev.tx_buffers[1]);
    outl(io_base + RTL8139_TSAD2, (u32)rtl8139_dev.tx_buffers[2]);
    outl(io_base + RTL8139_TSAD3, (u32)rtl8139_dev.tx_buffers[3]);
    
    /* Configure interrupts */
    outw(io_base + RTL8139_IMR, RTL8139_INT_ROK | RTL8139_INT_TOK | 
         RTL8139_INT_RER | RTL8139_INT_TER | RTL8139_INT_RXOVW);
    
    /* Configure receive */
    outl(io_base + RTL8139_RCR, RTL8139_RCR_AB | RTL8139_RCR_AM | 
         RTL8139_RCR_APM | RTL8139_RCR_WRAP | (0x06 << 13)); /* 8KB buffer */
    
    /* Configure transmit */
    outl(io_base + RTL8139_TCR, RTL8139_TCR_MXDMA | RTL8139_TCR_IFG);
    
    /* Enable receiver and transmitter */
    outb(io_base + RTL8139_CR, RTL8139_CR_RE | RTL8139_CR_TE);
    
    rtl8139_dev.rx_offset = 0;
    rtl8139_dev.tx_current = 0;
    rtl8139_dev.initialized = 1;
    
    return 1;
}

u8 rtl8139_send_packet(const void* data, u16 length) {
    if (!rtl8139_dev.initialized || length > TX_BUFFER_SIZE) return 0;
    
    u8 tx_desc = rtl8139_dev.tx_current;
    
    /* Wait for transmit descriptor to be available */
    u32 timeout = 1000000;
    while (timeout-- && !(inl(rtl8139_dev.io_base + RTL8139_TSD0 + tx_desc * 4) & RTL8139_TSD_OWN));
    
    if (!(inl(rtl8139_dev.io_base + RTL8139_TSD0 + tx_desc * 4) & RTL8139_TSD_OWN)) {
        return 0; /* Transmit descriptor not available */
    }
    
    /* Copy data to transmit buffer */
    const u8* src = (const u8*)data;
    for (u16 i = 0; i < length; i++) {
        rtl8139_dev.tx_buffers[tx_desc][i] = src[i];
    }
    
    /* Start transmission */
    outl(rtl8139_dev.io_base + RTL8139_TSD0 + tx_desc * 4, length);
    
    /* Move to next transmit descriptor */
    rtl8139_dev.tx_current = (rtl8139_dev.tx_current + 1) % 4;
    
    return 1;
}

u16 rtl8139_receive_packet(void* buffer, u16 max_length) {
    if (!rtl8139_dev.initialized) return 0;
    
    u16 capr = inw(rtl8139_dev.io_base + RTL8139_CAPR);
    u16 cbr = inw(rtl8139_dev.io_base + RTL8139_CBR);
    
    if (capr == cbr) {
        return 0; /* No packets available */
    }
    
    /* Packet header: status (2 bytes) + length (2 bytes) */
    u16 offset = (capr + 16) % RX_BUFFER_SIZE;
    u16 status = *(u16*)(rtl8139_dev.rx_buffer + offset);
    u16 length = *(u16*)(rtl8139_dev.rx_buffer + offset + 2);
    
    if (!(status & 0x01)) {
        return 0; /* Packet not valid */
    }
    
    /* Remove CRC from length */
    length -= 4;
    
    if (length > max_length) {
        length = max_length;
    }
    
    /* Copy packet data */
    u8* dest = (u8*)buffer;
    u16 data_offset = (offset + 4) % RX_BUFFER_SIZE;
    
    for (u16 i = 0; i < length; i++) {
        dest[i] = rtl8139_dev.rx_buffer[(data_offset + i) % RX_BUFFER_SIZE];
    }
    
    /* Update CAPR */
    u16 packet_size = ((length + 4 + 3) & ~3) + 4; /* Align to 4 bytes + header */
    capr = (capr + packet_size) % RX_BUFFER_SIZE;
    outw(rtl8139_dev.io_base + RTL8139_CAPR, capr - 16);
    
    return length;
}

void rtl8139_irq_handler(void) {
    if (!rtl8139_dev.initialized) return;
    
    u16 status = inw(rtl8139_dev.io_base + RTL8139_ISR);
    
    /* Clear interrupts */
    outw(rtl8139_dev.io_base + RTL8139_ISR, status);
    
    if (status & RTL8139_INT_ROK) {
        /* Packet received */
    }
    
    if (status & RTL8139_INT_TOK) {
        /* Packet transmitted */
    }
    
    if (status & RTL8139_INT_RER) {
        /* Receive error */
    }
    
    if (status & RTL8139_INT_TER) {
        /* Transmit error */
    }
    
    if (status & RTL8139_INT_RXOVW) {
        /* Receive buffer overflow */
        outb(rtl8139_dev.io_base + RTL8139_CR, RTL8139_CR_RE | RTL8139_CR_TE);
    }
}

void rtl8139_get_mac_address(u8* mac) {
    if (!rtl8139_dev.initialized) return;
    
    for (u8 i = 0; i < 6; i++) {
        mac[i] = rtl8139_dev.mac_addr[i];
    }
}

u8 rtl8139_is_link_up(void) {
    if (!rtl8139_dev.initialized) return 0;
    
    u16 bmsr = inw(rtl8139_dev.io_base + RTL8139_BMSR);
    return (bmsr & 0x04) != 0; /* Link status */
}

u8 rtl8139_is_initialized(void) {
    return rtl8139_dev.initialized;
}
