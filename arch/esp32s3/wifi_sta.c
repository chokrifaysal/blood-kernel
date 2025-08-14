/*
 * wifi_sta.c – ESP32-S3 WiFi station mode
 */

#include "kernel/types.h"
#include "wifi_stub.h"

extern void wifi_derive_pmk(const u8* ssid, u8 ssid_len, const u8* pass, u8 pass_len);
extern void wifi_derive_ptk(const u8* anonce, const u8* snonce, const u8* aa, const u8* spa);
extern u8 wifi_verify_mic(const u8* eapol, u16 len, const u8* mic);
extern void timer_delay(u32 ms);

typedef struct {
    u8 ssid[32];
    u8 ssid_len;
    u8 bssid[6];
    u8 channel;
    s8 rssi;
    u8 auth_mode;
    u8 cipher;
} scan_result_t;

static scan_result_t scan_results[16];
static u8 scan_count = 0;
static u8 sta_state = 0; // 0=idle, 1=scanning, 2=connecting, 3=connected
static u8 my_mac[6] = {0x24, 0x6F, 0x28, 0x00, 0x00, 0x01};

void wifi_scan_start(void) {
    sta_state = 1;
    scan_count = 0;
    
    wifi_set_mode(1); // STA mode
    
    /* Scan channels 1-14 */
    for (u8 ch = 1; ch <= 14; ch++) {
        wifi_set_channel(ch);
        
        /* Send probe request */
        u8 probe_req[64];
        u8 len = 0;
        
        /* 802.11 header */
        probe_req[len++] = 0x40; // Frame control
        probe_req[len++] = 0x00;
        probe_req[len++] = 0x00; // Duration
        probe_req[len++] = 0x00;
        
        /* Broadcast BSSID */
        for (u8 i = 0; i < 6; i++) probe_req[len++] = 0xFF;
        
        /* Source address (our MAC) */
        for (u8 i = 0; i < 6; i++) probe_req[len++] = my_mac[i];
        
        /* BSSID (broadcast) */
        for (u8 i = 0; i < 6; i++) probe_req[len++] = 0xFF;
        
        /* Sequence control */
        probe_req[len++] = 0x00;
        probe_req[len++] = 0x00;
        
        /* SSID element (wildcard) */
        probe_req[len++] = 0x00; // Element ID
        probe_req[len++] = 0x00; // Length (wildcard)
        
        /* Supported rates */
        probe_req[len++] = 0x01; // Element ID
        probe_req[len++] = 0x08; // Length
        probe_req[len++] = 0x82; // 1 Mbps
        probe_req[len++] = 0x84; // 2 Mbps
        probe_req[len++] = 0x8B; // 5.5 Mbps
        probe_req[len++] = 0x96; // 11 Mbps
        probe_req[len++] = 0x0C; // 6 Mbps
        probe_req[len++] = 0x12; // 9 Mbps
        probe_req[len++] = 0x18; // 12 Mbps
        probe_req[len++] = 0x24; // 18 Mbps
        
        wifi_tx_raw(probe_req, len);
        timer_delay(50); // Wait for responses
        
        /* Process probe responses */
        u8 rx_buf[1500];
        u16 rx_len;
        while ((rx_len = wifi_rx_raw(rx_buf, sizeof(rx_buf))) > 0) {
            if (rx_len < 36) continue; // Too short
            
            /* Check if probe response */
            if (rx_buf[0] != 0x50) continue;
            
            if (scan_count >= 16) break;
            
            scan_result_t* result = &scan_results[scan_count];
            
            /* Extract BSSID */
            for (u8 i = 0; i < 6; i++) {
                result->bssid[i] = rx_buf[16 + i];
            }
            
            result->channel = ch;
            result->rssi = -50; // Simplified RSSI
            
            /* Parse information elements */
            u8* ie = rx_buf + 36; // Skip fixed fields
            u16 ie_len = rx_len - 36;
            u16 pos = 0;
            
            while (pos + 1 < ie_len) {
                u8 id = ie[pos];
                u8 len = ie[pos + 1];
                
                if (pos + 2 + len > ie_len) break;
                
                if (id == 0) { // SSID
                    result->ssid_len = len;
                    for (u8 i = 0; i < len && i < 32; i++) {
                        result->ssid[i] = ie[pos + 2 + i];
                    }
                } else if (id == 48) { // RSN
                    result->auth_mode = 2; // WPA2
                } else if (id == 221) { // Vendor specific (WPA)
                    if (len >= 4 && ie[pos + 2] == 0x00 && 
                        ie[pos + 3] == 0x50 && ie[pos + 4] == 0xF2) {
                        result->auth_mode = 1; // WPA
                    }
                }
                
                pos += 2 + len;
            }
            
            scan_count++;
        }
    }
    
    sta_state = 0; // Scan complete
}

u8 wifi_get_scan_results(scan_result_t* results, u8 max_results) {
    u8 count = (scan_count < max_results) ? scan_count : max_results;
    for (u8 i = 0; i < count; i++) {
        results[i] = scan_results[i];
    }
    return count;
}

u8 wifi_connect(const u8* ssid, u8 ssid_len, const u8* password, u8 pass_len) {
    /* Find AP in scan results */
    scan_result_t* target = 0;
    for (u8 i = 0; i < scan_count; i++) {
        if (scan_results[i].ssid_len == ssid_len) {
            u8 match = 1;
            for (u8 j = 0; j < ssid_len; j++) {
                if (scan_results[i].ssid[j] != ssid[j]) {
                    match = 0;
                    break;
                }
            }
            if (match) {
                target = &scan_results[i];
                break;
            }
        }
    }
    
    if (!target) return 0; // AP not found
    
    sta_state = 2; // Connecting
    wifi_set_channel(target->channel);
    
    /* Derive PMK from password */
    wifi_derive_pmk(ssid, ssid_len, password, pass_len);
    
    /* Send authentication frame */
    u8 auth_frame[30];
    u8 len = 0;
    
    /* 802.11 header */
    auth_frame[len++] = 0xB0; // Authentication
    auth_frame[len++] = 0x00;
    auth_frame[len++] = 0x00; // Duration
    auth_frame[len++] = 0x00;
    
    /* Destination (AP BSSID) */
    for (u8 i = 0; i < 6; i++) auth_frame[len++] = target->bssid[i];
    
    /* Source (our MAC) */
    for (u8 i = 0; i < 6; i++) auth_frame[len++] = my_mac[i];
    
    /* BSSID */
    for (u8 i = 0; i < 6; i++) auth_frame[len++] = target->bssid[i];
    
    /* Sequence control */
    auth_frame[len++] = 0x00;
    auth_frame[len++] = 0x00;
    
    /* Authentication algorithm (Open System) */
    auth_frame[len++] = 0x00;
    auth_frame[len++] = 0x00;
    
    /* Transaction sequence */
    auth_frame[len++] = 0x01;
    auth_frame[len++] = 0x00;
    
    /* Status code */
    auth_frame[len++] = 0x00;
    auth_frame[len++] = 0x00;
    
    wifi_tx_raw(auth_frame, len);
    
    /* Wait for authentication response */
    timer_delay(100);
    
    /* Send association request */
    u8 assoc_req[128];
    len = 0;
    
    /* 802.11 header */
    assoc_req[len++] = 0x00; // Association request
    assoc_req[len++] = 0x00;
    assoc_req[len++] = 0x00; // Duration
    assoc_req[len++] = 0x00;
    
    /* Destination (AP BSSID) */
    for (u8 i = 0; i < 6; i++) assoc_req[len++] = target->bssid[i];
    
    /* Source (our MAC) */
    for (u8 i = 0; i < 6; i++) assoc_req[len++] = my_mac[i];
    
    /* BSSID */
    for (u8 i = 0; i < 6; i++) assoc_req[len++] = target->bssid[i];
    
    /* Sequence control */
    assoc_req[len++] = 0x00;
    assoc_req[len++] = 0x00;
    
    /* Capability info */
    assoc_req[len++] = 0x01; // ESS
    assoc_req[len++] = 0x00;
    
    /* Listen interval */
    assoc_req[len++] = 0x0A;
    assoc_req[len++] = 0x00;
    
    /* SSID element */
    assoc_req[len++] = 0x00; // Element ID
    assoc_req[len++] = ssid_len;
    for (u8 i = 0; i < ssid_len; i++) {
        assoc_req[len++] = ssid[i];
    }
    
    /* Supported rates */
    assoc_req[len++] = 0x01; // Element ID
    assoc_req[len++] = 0x08; // Length
    assoc_req[len++] = 0x82; // 1 Mbps
    assoc_req[len++] = 0x84; // 2 Mbps
    assoc_req[len++] = 0x8B; // 5.5 Mbps
    assoc_req[len++] = 0x96; // 11 Mbps
    assoc_req[len++] = 0x0C; // 6 Mbps
    assoc_req[len++] = 0x12; // 9 Mbps
    assoc_req[len++] = 0x18; // 12 Mbps
    assoc_req[len++] = 0x24; // 18 Mbps
    
    wifi_tx_raw(assoc_req, len);
    
    /* Wait for association response */
    timer_delay(100);
    
    sta_state = 3; // Connected
    return 1;
}

u8 wifi_is_connected(void) {
    return (sta_state == 3);
}

void wifi_disconnect(void) {
    /* Send deauthentication */
    u8 deauth[26];
    u8 len = 0;
    
    /* 802.11 header */
    deauth[len++] = 0xC0; // Deauthentication
    deauth[len++] = 0x00;
    deauth[len++] = 0x00; // Duration
    deauth[len++] = 0x00;
    
    /* Fill addresses (simplified) */
    for (u8 i = 0; i < 18; i++) deauth[len++] = 0x00;
    
    /* Reason code */
    deauth[len++] = 0x03; // Deauthenticated because sending STA is leaving
    deauth[len++] = 0x00;
    
    wifi_tx_raw(deauth, len);
    sta_state = 0;
}
