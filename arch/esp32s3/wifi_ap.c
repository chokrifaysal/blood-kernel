/*
 * wifi_ap.c – ESP32-S3 WiFi access point mode
 */

#include "kernel/types.h"
#include "wifi_stub.h"

extern void timer_delay(u32 ms);

typedef struct {
    u8 mac[6];
    u8 aid;
    u8 state; // 0=none, 1=auth, 2=assoc
    u32 last_seen;
} sta_info_t;

static sta_info_t stations[8];
static u8 sta_count = 0;
static u8 ap_ssid[32] = "ESP32-S3-AP";
static u8 ap_ssid_len = 11;
static u8 ap_channel = 6;
static u8 ap_bssid[6] = {0x24, 0x6F, 0x28, 0xAP, 0xAP, 0xAP};
static u32 beacon_seq = 0;

void wifi_ap_init(const u8* ssid, u8 ssid_len, u8 channel) {
    if (ssid_len > 32) ssid_len = 32;
    
    ap_ssid_len = ssid_len;
    for (u8 i = 0; i < ssid_len; i++) {
        ap_ssid[i] = ssid[i];
    }
    
    ap_channel = channel;
    wifi_set_mode(2); // AP mode
    wifi_set_channel(channel);
    
    /* Clear station list */
    for (u8 i = 0; i < 8; i++) {
        stations[i].state = 0;
    }
    sta_count = 0;
}

void wifi_ap_send_beacon(void) {
    u8 beacon[128];
    u8 len = 0;
    
    /* 802.11 header */
    beacon[len++] = 0x80; // Beacon
    beacon[len++] = 0x00;
    beacon[len++] = 0x00; // Duration
    beacon[len++] = 0x00;
    
    /* Destination (broadcast) */
    for (u8 i = 0; i < 6; i++) beacon[len++] = 0xFF;
    
    /* Source (our BSSID) */
    for (u8 i = 0; i < 6; i++) beacon[len++] = ap_bssid[i];
    
    /* BSSID */
    for (u8 i = 0; i < 6; i++) beacon[len++] = ap_bssid[i];
    
    /* Sequence control */
    beacon[len++] = (beacon_seq & 0xFF);
    beacon[len++] = ((beacon_seq >> 8) & 0x0F);
    beacon_seq++;
    
    /* Fixed parameters */
    /* Timestamp */
    for (u8 i = 0; i < 8; i++) beacon[len++] = 0x00;
    
    /* Beacon interval (100 TU = 102.4 ms) */
    beacon[len++] = 0x64;
    beacon[len++] = 0x00;
    
    /* Capability info */
    beacon[len++] = 0x01; // ESS
    beacon[len++] = 0x00;
    
    /* Information elements */
    /* SSID */
    beacon[len++] = 0x00; // Element ID
    beacon[len++] = ap_ssid_len;
    for (u8 i = 0; i < ap_ssid_len; i++) {
        beacon[len++] = ap_ssid[i];
    }
    
    /* Supported rates */
    beacon[len++] = 0x01; // Element ID
    beacon[len++] = 0x08; // Length
    beacon[len++] = 0x82; // 1 Mbps (basic)
    beacon[len++] = 0x84; // 2 Mbps (basic)
    beacon[len++] = 0x8B; // 5.5 Mbps (basic)
    beacon[len++] = 0x96; // 11 Mbps (basic)
    beacon[len++] = 0x0C; // 6 Mbps
    beacon[len++] = 0x12; // 9 Mbps
    beacon[len++] = 0x18; // 12 Mbps
    beacon[len++] = 0x24; // 18 Mbps
    
    /* DS Parameter Set (channel) */
    beacon[len++] = 0x03; // Element ID
    beacon[len++] = 0x01; // Length
    beacon[len++] = ap_channel;
    
    /* TIM (Traffic Indication Map) */
    beacon[len++] = 0x05; // Element ID
    beacon[len++] = 0x04; // Length
    beacon[len++] = 0x00; // DTIM count
    beacon[len++] = 0x01; // DTIM period
    beacon[len++] = 0x00; // Bitmap control
    beacon[len++] = 0x00; // Partial virtual bitmap
    
    wifi_tx_raw(beacon, len);
}

static sta_info_t* find_station(const u8* mac) {
    for (u8 i = 0; i < 8; i++) {
        if (stations[i].state > 0) {
            u8 match = 1;
            for (u8 j = 0; j < 6; j++) {
                if (stations[i].mac[j] != mac[j]) {
                    match = 0;
                    break;
                }
            }
            if (match) return &stations[i];
        }
    }
    return 0;
}

static sta_info_t* add_station(const u8* mac) {
    /* Find free slot */
    for (u8 i = 0; i < 8; i++) {
        if (stations[i].state == 0) {
            for (u8 j = 0; j < 6; j++) {
                stations[i].mac[j] = mac[j];
            }
            stations[i].aid = i + 1;
            stations[i].state = 1; // Authenticated
            sta_count++;
            return &stations[i];
        }
    }
    return 0; // No free slots
}

void wifi_ap_handle_auth(const u8* frame, u16 len) {
    if (len < 30) return;
    
    const u8* src_mac = frame + 10;
    u16 auth_alg = *(u16*)(frame + 24);
    u16 auth_seq = *(u16*)(frame + 26);
    
    if (auth_alg != 0 || auth_seq != 1) return; // Only open system, seq 1
    
    /* Add or update station */
    sta_info_t* sta = find_station(src_mac);
    if (!sta) {
        sta = add_station(src_mac);
    }
    
    if (!sta) return; // No space
    
    /* Send authentication response */
    u8 auth_resp[30];
    u8 resp_len = 0;
    
    /* 802.11 header */
    auth_resp[resp_len++] = 0xB0; // Authentication
    auth_resp[resp_len++] = 0x00;
    auth_resp[resp_len++] = 0x00; // Duration
    auth_resp[resp_len++] = 0x00;
    
    /* Destination (station MAC) */
    for (u8 i = 0; i < 6; i++) auth_resp[resp_len++] = src_mac[i];
    
    /* Source (our BSSID) */
    for (u8 i = 0; i < 6; i++) auth_resp[resp_len++] = ap_bssid[i];
    
    /* BSSID */
    for (u8 i = 0; i < 6; i++) auth_resp[resp_len++] = ap_bssid[i];
    
    /* Sequence control */
    auth_resp[resp_len++] = 0x00;
    auth_resp[resp_len++] = 0x00;
    
    /* Authentication algorithm */
    auth_resp[resp_len++] = 0x00;
    auth_resp[resp_len++] = 0x00;
    
    /* Transaction sequence */
    auth_resp[resp_len++] = 0x02;
    auth_resp[resp_len++] = 0x00;
    
    /* Status code (success) */
    auth_resp[resp_len++] = 0x00;
    auth_resp[resp_len++] = 0x00;
    
    wifi_tx_raw(auth_resp, resp_len);
}

void wifi_ap_handle_assoc(const u8* frame, u16 len) {
    if (len < 28) return;
    
    const u8* src_mac = frame + 10;
    
    /* Find authenticated station */
    sta_info_t* sta = find_station(src_mac);
    if (!sta || sta->state != 1) return;
    
    /* Mark as associated */
    sta->state = 2;
    
    /* Send association response */
    u8 assoc_resp[32];
    u8 resp_len = 0;
    
    /* 802.11 header */
    assoc_resp[resp_len++] = 0x10; // Association response
    assoc_resp[resp_len++] = 0x00;
    assoc_resp[resp_len++] = 0x00; // Duration
    assoc_resp[resp_len++] = 0x00;
    
    /* Destination (station MAC) */
    for (u8 i = 0; i < 6; i++) assoc_resp[resp_len++] = src_mac[i];
    
    /* Source (our BSSID) */
    for (u8 i = 0; i < 6; i++) assoc_resp[resp_len++] = ap_bssid[i];
    
    /* BSSID */
    for (u8 i = 0; i < 6; i++) assoc_resp[resp_len++] = ap_bssid[i];
    
    /* Sequence control */
    assoc_resp[resp_len++] = 0x00;
    assoc_resp[resp_len++] = 0x00;
    
    /* Capability info */
    assoc_resp[resp_len++] = 0x01;
    assoc_resp[resp_len++] = 0x00;
    
    /* Status code (success) */
    assoc_resp[resp_len++] = 0x00;
    assoc_resp[resp_len++] = 0x00;
    
    /* Association ID */
    assoc_resp[resp_len++] = sta->aid;
    assoc_resp[resp_len++] = 0xC0; // AID with bits 14-15 set
    
    wifi_tx_raw(assoc_resp, resp_len);
}

void wifi_ap_process_frame(const u8* frame, u16 len) {
    if (len < 24) return;
    
    u8 frame_type = frame[0] & 0xFC;
    
    switch (frame_type) {
        case 0xB0: // Authentication
            wifi_ap_handle_auth(frame, len);
            break;
        case 0x00: // Association request
            wifi_ap_handle_assoc(frame, len);
            break;
        case 0x40: // Probe request
            /* Could send probe response here */
            break;
        case 0xC0: // Deauthentication
            /* Remove station */
            break;
    }
}

u8 wifi_ap_get_station_count(void) {
    return sta_count;
}

void wifi_ap_task(void) {
    static u32 last_beacon = 0;
    u32 now = 0; // Would use timer_ticks()
    
    /* Send beacon every 100ms */
    if (now - last_beacon >= 100) {
        wifi_ap_send_beacon();
        last_beacon = now;
    }
    
    /* Process incoming frames */
    u8 rx_buf[1500];
    u16 rx_len = wifi_rx_raw(rx_buf, sizeof(rx_buf));
    if (rx_len > 0) {
        wifi_ap_process_frame(rx_buf, rx_len);
    }
}
