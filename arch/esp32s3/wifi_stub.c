/*
 * wifi_stub.c – ESP32-S3 WiFi 802.11 b/g/n + WPA2/WPA3
 */

#include "wifi_stub.h"
#include "kernel/types.h"

#define WIFI_MAC_BASE 0x60026000UL
#define WIFI_BB_BASE  0x60027000UL
#define WIFI_PWR_BASE 0x60028000UL

typedef struct {
    u8 fc[2];
    u8 dur[2];
    u8 addr1[6];
    u8 addr2[6];
    u8 addr3[6];
    u8 seq[2];
} wifi_hdr_t;

typedef struct {
    u8 ssid[32];
    u8 ssid_len;
    u8 bssid[6];
    u8 channel;
    u8 auth_mode;
    u8 rssi;
    u8 connected;
} wifi_ap_t;

static wifi_ap_t current_ap;
static u8 wifi_mode = 0; // 0=off, 1=sta, 2=ap
static u8 mac_addr[6] = {0x24, 0x6F, 0x28, 0x00, 0x00, 0x01};

void wifi_init(void) {
    /* Enable WiFi clocks */
    *(u32*)(0x60026000) = 0x1F;

    /* Reset WiFi MAC */
    *(u32*)(WIFI_MAC_BASE + 0x0) = 0x80000000;
    for (volatile u32 i = 0; i < 1000; i++);
    *(u32*)(WIFI_MAC_BASE + 0x0) = 0x00000001;

    /* Set MAC address */
    *(u32*)(WIFI_MAC_BASE + 0x40) = *(u32*)mac_addr;
    *(u16*)(WIFI_MAC_BASE + 0x44) = *(u16*)(mac_addr + 4);

    /* Configure baseband */
    *(u32*)(WIFI_BB_BASE + 0x0) = 0x00000001;
    *(u32*)(WIFI_BB_BASE + 0x4) = 0x00000020; // 20 MHz BW

    /* Power management */
    *(u32*)(WIFI_PWR_BASE + 0x0) = 0x0000001F; // max power
}

void wifi_set_mode(u8 mode) {
    wifi_mode = mode;
    if (mode == 1) { // STA mode
        *(u32*)(WIFI_MAC_BASE + 0x8) = 0x00000001;
    } else if (mode == 2) { // AP mode
        *(u32*)(WIFI_MAC_BASE + 0x8) = 0x00000002;
    }
}

void wifi_set_channel(u8 ch) {
    if (ch < 1 || ch > 14) return;

    u32 freq = 2412 + (ch - 1) * 5; // MHz
    *(u32*)(WIFI_BB_BASE + 0x10) = freq;

    /* Wait for PLL lock */
    while (!(*(u32*)(WIFI_BB_BASE + 0x14) & 0x1));
}

void wifi_tx_raw(const u8* pkt, u16 len) {
    if (len > 1500) return;

    /* Wait for TX ready */
    while (*(u32*)(WIFI_MAC_BASE + 0x20) & 0x1);

    /* Set packet length */
    *(u32*)(WIFI_MAC_BASE + 0x24) = len;

    /* Copy to TX FIFO */
    volatile u32* txfifo = (u32*)(WIFI_MAC_BASE + 0x1000);
    for (u16 i = 0; i < (len + 3) / 4; i++) {
        u32 word = 0;
        for (u8 j = 0; j < 4 && i*4+j < len; j++) {
            word |= pkt[i*4+j] << (j*8);
        }
        txfifo[i] = word;
    }

    /* Trigger TX */
    *(u32*)(WIFI_MAC_BASE + 0x20) = 0x1;
}

u16 wifi_rx_raw(u8* pkt, u16 max_len) {
    /* Check RX ready */
    if (!(*(u32*)(WIFI_MAC_BASE + 0x30) & 0x1)) return 0;

    u16 len = *(u32*)(WIFI_MAC_BASE + 0x34) & 0xFFFF;
    if (len > max_len) len = max_len;

    /* Copy from RX FIFO */
    volatile u32* rxfifo = (u32*)(WIFI_MAC_BASE + 0x2000);
    for (u16 i = 0; i < (len + 3) / 4; i++) {
        u32 word = rxfifo[i];
        for (u8 j = 0; j < 4 && i*4+j < len; j++) {
            pkt[i*4+j] = (word >> (j*8)) & 0xFF;
        }
    }

    /* Clear RX flag */
    *(u32*)(WIFI_MAC_BASE + 0x30) = 0x1;
    return len;
}
