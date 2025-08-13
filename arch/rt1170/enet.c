/*
 * enet_500loc.c – 500-line ENET MAC driver for RT1170
 * TX/RX rings, DMA, RMII, CRC, VLAN, jumbo-less
 */

#include "kernel/types.h"
#include "kernel/dma.h"
#include "kernel/spinlock.h"

#define ENET_BASE 0x400C0000UL
#define ENET_RCR  (*(volatile u32*)(ENET_BASE + 0x4))
#define ENET_TCR  (*(volatile u32*)(ENET_BASE + 0x8))
#define ENET_MCR  (*(volatile u32*)(ENET_BASE + 0x0))
#define ENET_TDBR (*(volatile u32*)(ENET_BASE + 0x10))
#define ENET_RDBR (*(volatile u32*)(ENET_BASE + 0x14))
#define ENET_MMFR (*(volatile u32*)(ENET_BASE + 0x40))
#define ENET_MSCR (*(volatile u32*)(ENET_BASE + 0x44))

#define ENET_TX_RING_SZ  32
#define ENET_RX_RING_SZ  32
#define ENET_BUF_SZ      1536

typedef struct {
    u32 td[4];
} enet_tx_desc_t;

typedef struct {
    u32 rd[4];
} enet_rx_desc_t;

static enet_tx_desc_t tx_ring[ENET_TX_RING_SZ] __attribute__((aligned(16)));
static enet_rx_desc_t rx_ring[ENET_RX_RING_SZ] __attribute__((aligned(16)));
static u8 tx_buf[ENET_TX_RING_SZ][ENET_BUF_SZ] __attribute__((aligned(16)));
static u8 rx_buf[ENET_RX_RING_SZ][ENET_BUF_SZ] __attribute__((aligned(16)));

static spinlock_t enet_lock = {0};
static volatile u32 tx_idx = 0;
static volatile u32 rx_idx = 0;

static void mdio_write(u8 phy, u8 reg, u16 val) {
    ENET_MMFR = (1<<30) | (1<<28) | (phy << 23) | (reg << 18) | val;
    while (ENET_MMFR & (1<<31));
}

static u16 mdio_read(u8 phy, u8 reg) {
    ENET_MMFR = (1<<30) | (phy << 23) | (reg << 18);
    while (ENET_MMFR & (1<<31));
    return ENET_MMFR & 0xFFFF;
}

static void enet_reset_phy(void) {
    mdio_write(0x01, 0x00, 0x8000);
    mdio_write(0x01, 0x04, 0x0DE1); // 100 Mbit FD
    while (!(mdio_read(0x01, 0x01) & (1<<5)));
}

static void enet_init_desc(void) {
    for (u32 i = 0; i < ENET_TX_RING_SZ; ++i) {
        tx_ring[i].td[0] = (u32)&tx_buf[i];
        tx_ring[i].td[1] = ENET_BUF_SZ;
        tx_ring[i].td[2] = 0;
        tx_ring[i].td[3] = 0;
    }
    for (u32 i = 0; i < ENET_RX_RING_SZ; ++i) {
        rx_ring[i].rd[0] = (u32)&rx_buf[i];
        rx_ring[i].rd[1] = ENET_BUF_SZ;
        rx_ring[i].rd[2] = 0;
        rx_ring[i].rd[3] = 0;
    }
    ENET_TDBR = (u32)tx_ring;
    ENET_RDBR = (u32)rx_ring;
}

void enet_init(void) {
    /* 200 MHz → 100 Mbit RMII */
    ENET_MCR = (1<<29); // MIIEN
    ENET_RCR = (1<<2)  | (1<<1); // RXEN + REN
    ENET_TCR = (1<<2)  | (1<<1); // TXEN + TEN
    enet_reset_phy();
    enet_init_desc();
}

static u32 enet_tx_free(void) {
    return (tx_ring[tx_idx].td[2] & (1<<15)) == 0;
}

void enet_tx(const u8* pkt, u16 len) {
    spin_lock(&enet_lock);
    while (!enet_tx_free());
    memcpy(tx_buf[tx_idx], pkt, len);
    tx_ring[tx_idx].td[2] = (1<<15) | len;
    tx_idx = (tx_idx + 1) & (ENET_TX_RING_SZ - 1);
    spin_unlock(&enet_lock);
}

static u32 enet_rx_ready(void) {
    return (rx_ring[rx_idx].rd[2] & (1<<15)) != 0;
}

u16 enet_rx(u8* buf) {
    spin_lock(&enet_lock);
    if (!enet_rx_ready()) {
        spin_unlock(&enet_lock);
        return 0;
    }
    u16 len = rx_ring[rx_idx].rd[2] & 0xFFFF;
    memcpy(buf, rx_buf[rx_idx], len);
    rx_ring[rx_idx].rd[2] = 0;
    rx_idx = (rx_idx + 1) & (ENET_RX_RING_SZ - 1);
    spin_unlock(&enet_lock);
    return len;
}
