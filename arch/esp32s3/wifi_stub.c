/*
 * wifi_stub.c – 802.11 raw frame stub (no stack, no heap)
 */

#include "wifi_stub.h"
#include "kernel/types.h"

#define WIFI_MAC_BASE 0x60026000UL

void wifi_init(void) {
    *(u32*)(WIFI_MAC_BASE + 0x0) = 0x00000001; // enable MAC
}

void wifi_tx_raw(const u8* pkt, u16 len) {
    volatile u32* const TXFIFO = (u32*)(WIFI_MAC_BASE + 0x1000);
    for (u16 i = 0; i < len; i += 4) {
        u32 word = *(u32*)(pkt + i);
        *TXFIFO = word;
    }
}
