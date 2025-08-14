/*
 * eth_phy.c – STM32H745 Ethernet PHY LAN8742A
 */

#include "kernel/types.h"

#define ETH_BASE 0x40028000UL
#define RCC_AHB1ENR (*(volatile u32*)0x58024530UL)

typedef volatile struct {
    u32 MACCR;
    u32 MACECR;
    u32 MACPFR;
    u32 MACWTR;
    u32 MACHT0R;
    u32 MACHT1R;
    u32 _pad1[14];
    u32 MACVTR;
    u32 _pad2;
    u32 MACVHTR;
    u32 _pad3;
    u32 MACVIR;
    u32 MACIVIR;
    u32 _pad4[2];
    u32 MACTFCR;
    u32 _pad5[7];
    u32 MACRFCR;
    u32 _pad6[7];
    u32 MACISR;
    u32 MACIER;
    u32 MACRXTXSR;
    u32 _pad7;
    u32 MACPCSR;
    u32 MACRWKPFR;
    u32 _pad8[2];
    u32 MACLCSR;
    u32 MACLTCR;
    u32 MACLETR;
    u32 MAC1USTCR;
    u32 _pad9[12];
    u32 MACVR;
    u32 MACDR;
    u32 _pad10[2];
    u32 MACHWF1R;
    u32 MACHWF2R;
    u32 _pad11[54];
    u32 MACMDIOAR;
    u32 MACMDIODR;
    u32 _pad12[2];
    u32 MACARPAR;
    u32 _pad13[59];
    u32 MACA0HR;
    u32 MACA0LR;
} ETH_TypeDef;

static ETH_TypeDef* const ETH = (ETH_TypeDef*)ETH_BASE;

/* PHY registers */
#define PHY_BCR     0x00
#define PHY_BSR     0x01
#define PHY_ID1     0x02
#define PHY_ID2     0x03
#define PHY_ANAR    0x04
#define PHY_ANLPAR  0x05
#define PHY_ANER    0x06
#define PHY_ANNPTR  0x07
#define PHY_ANLPRNP 0x08

#define PHY_ADDR    0x00

static u16 phy_read(u8 reg) {
    /* Wait for MDIO ready */
    while (ETH->MACMDIOAR & (1<<0));
    
    /* Setup read operation */
    ETH->MACMDIOAR = (PHY_ADDR << 21) | (reg << 16) | (3 << 2) | (1 << 0);
    
    /* Wait for completion */
    while (ETH->MACMDIOAR & (1<<0));
    
    return ETH->MACMDIODR & 0xFFFF;
}

static void phy_write(u8 reg, u16 val) {
    /* Wait for MDIO ready */
    while (ETH->MACMDIOAR & (1<<0));
    
    /* Write data */
    ETH->MACMDIODR = val;
    
    /* Setup write operation */
    ETH->MACMDIOAR = (PHY_ADDR << 21) | (reg << 16) | (1 << 2) | (1 << 0);
    
    /* Wait for completion */
    while (ETH->MACMDIOAR & (1<<0));
}

void eth_phy_init(void) {
    /* Enable ETH clock */
    RCC_AHB1ENR |= (1<<15) | (1<<16) | (1<<17);
    
    /* MDIO clock = HCLK/102 = ~2.35 MHz */
    ETH->MACMDIOAR = (4 << 8);
    
    /* Reset PHY */
    phy_write(PHY_BCR, (1<<15));
    
    /* Wait for reset complete */
    while (phy_read(PHY_BCR) & (1<<15));
    
    /* Auto-negotiation enable */
    phy_write(PHY_BCR, (1<<12));
    
    /* Advertise 100M full duplex */
    phy_write(PHY_ANAR, (1<<8) | (1<<6) | (1<<5) | 0x01);
    
    /* Restart auto-negotiation */
    phy_write(PHY_BCR, (1<<12) | (1<<9));
}

u8 eth_phy_link_up(void) {
    return (phy_read(PHY_BSR) >> 2) & 1;
}

u8 eth_phy_speed_100m(void) {
    u16 anlpar = phy_read(PHY_ANLPAR);
    return (anlpar >> 8) & 1;
}

u8 eth_phy_full_duplex(void) {
    u16 anlpar = phy_read(PHY_ANLPAR);
    return (anlpar >> 6) & 1;
}

void eth_phy_get_status(u8* link, u8* speed, u8* duplex) {
    u16 bsr = phy_read(PHY_BSR);
    u16 anlpar = phy_read(PHY_ANLPAR);
    
    *link = (bsr >> 2) & 1;
    *speed = (anlpar >> 8) & 1;  // 1=100M, 0=10M
    *duplex = (anlpar >> 6) & 1; // 1=full, 0=half
}
