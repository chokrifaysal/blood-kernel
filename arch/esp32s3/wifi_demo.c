/*
 * wifi_demo.c – ESP32-S3 WiFi demo tasks
 */

#include "kernel/types.h"
#include "wifi_stub.h"

extern void timer_delay(u32 ms);

static void wifi_sta_demo_task(void) {
    scan_result_t results[16];
    u8 count;
    
    wifi_init();
    wifi_set_mode(WIFI_MODE_STA);
    
    while (1) {
        /* Scan for networks */
        wifi_scan_start();
        timer_delay(3000); // Wait for scan
        
        count = wifi_get_scan_results(results, 16);
        
        /* Try to connect to first WPA2 network */
        for (u8 i = 0; i < count; i++) {
            if (results[i].auth_mode == WIFI_AUTH_WPA2) {
                /* Try common password */
                const u8 password[] = "password123";
                if (wifi_connect(results[i].ssid, results[i].ssid_len, 
                               password, sizeof(password) - 1)) {
                    /* Connected successfully */
                    while (wifi_is_connected()) {
                        timer_delay(1000);
                    }
                }
                break;
            }
        }
        
        timer_delay(10000); // Wait before next scan
    }
}

static void wifi_ap_demo_task(void) {
    const u8 ap_ssid[] = "ESP32-S3-Demo";
    
    wifi_init();
    wifi_ap_init(ap_ssid, sizeof(ap_ssid) - 1, 6);
    
    while (1) {
        wifi_ap_task(); // Process AP events
        timer_delay(10);
    }
}

static void wifi_raw_demo_task(void) {
    u8 beacon[128];
    u8 len = 0;
    
    wifi_init();
    wifi_set_mode(WIFI_MODE_AP);
    wifi_set_channel(11);
    
    /* Build custom beacon frame */
    beacon[len++] = 0x80; // Beacon frame
    beacon[len++] = 0x00;
    beacon[len++] = 0x00; // Duration
    beacon[len++] = 0x00;
    
    /* Broadcast destination */
    for (u8 i = 0; i < 6; i++) beacon[len++] = 0xFF;
    
    /* Source MAC */
    beacon[len++] = 0x24; beacon[len++] = 0x6F; beacon[len++] = 0x28;
    beacon[len++] = 0xDE; beacon[len++] = 0xMO; beacon[len++] = 0x01;
    
    /* BSSID */
    beacon[len++] = 0x24; beacon[len++] = 0x6F; beacon[len++] = 0x28;
    beacon[len++] = 0xDE; beacon[len++] = 0xMO; beacon[len++] = 0x01;
    
    /* Sequence control */
    beacon[len++] = 0x00; beacon[len++] = 0x00;
    
    /* Fixed parameters */
    for (u8 i = 0; i < 8; i++) beacon[len++] = 0x00; // Timestamp
    beacon[len++] = 0x64; beacon[len++] = 0x00;      // Beacon interval
    beacon[len++] = 0x01; beacon[len++] = 0x00;      // Capability
    
    /* SSID element */
    beacon[len++] = 0x00; // Element ID
    beacon[len++] = 0x0B; // Length
    const u8 ssid[] = "RAW-BEACON";
    for (u8 i = 0; i < 11; i++) beacon[len++] = ssid[i];
    
    /* Supported rates */
    beacon[len++] = 0x01; // Element ID
    beacon[len++] = 0x08; // Length
    beacon[len++] = 0x82; beacon[len++] = 0x84;
    beacon[len++] = 0x8B; beacon[len++] = 0x96;
    beacon[len++] = 0x0C; beacon[len++] = 0x12;
    beacon[len++] = 0x18; beacon[len++] = 0x24;
    
    /* Channel */
    beacon[len++] = 0x03; // Element ID
    beacon[len++] = 0x01; // Length
    beacon[len++] = 0x0B; // Channel 11
    
    while (1) {
        wifi_tx_raw(beacon, len);
        timer_delay(100); // Send beacon every 100ms
    }
}

static void wifi_sniffer_task(void) {
    u8 rx_buf[1500];
    u16 rx_len;
    u32 pkt_count = 0;
    
    wifi_init();
    wifi_set_mode(WIFI_MODE_STA);
    
    /* Monitor all channels */
    for (u8 ch = 1; ch <= 14; ch++) {
        wifi_set_channel(ch);
        
        /* Capture packets for 1 second */
        for (u16 i = 0; i < 100; i++) {
            rx_len = wifi_rx_raw(rx_buf, sizeof(rx_buf));
            if (rx_len > 0) {
                pkt_count++;
                
                /* Basic frame analysis */
                if (rx_len >= 24) {
                    u8 frame_type = rx_buf[0] & 0xFC;
                    switch (frame_type) {
                        case 0x80: // Beacon
                            /* Extract SSID from beacon */
                            break;
                        case 0x40: // Probe request
                            /* Monitor probe requests */
                            break;
                        case 0x08: // Data frame
                            /* Monitor data traffic */
                            break;
                    }
                }
            }
            timer_delay(10);
        }
    }
}

static void wifi_wpa3_demo_task(void) {
    u8 commit_msg[96];
    u8 confirm_msg[16];
    u8 peer_mac[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    const u8 password[] = "wpa3password";
    
    wifi_init();
    wifi_set_mode(WIFI_MODE_STA);
    
    while (1) {
        /* WPA3 SAE handshake demo */
        u8 commit_len = wifi_wpa3_sae_commit(peer_mac, password, commit_msg);
        if (commit_len > 0) {
            /* Send commit message */
            wifi_tx_raw(commit_msg, commit_len);
            
            /* Generate confirm message */
            u8 confirm_len = wifi_wpa3_sae_confirm(commit_msg, confirm_msg);
            if (confirm_len > 0) {
                wifi_tx_raw(confirm_msg, confirm_len);
            }
        }
        
        timer_delay(5000);
    }
}

void esp32s3_wifi_demo_init(void) {
    /* Called from kernel_main for ESP32-S3 */
    extern void task_create(void (*func)(void), u8 prio, u16 stack_size);
    
    /* Create different WiFi demo tasks */
    task_create(wifi_sta_demo_task, 1, 512);
    task_create(wifi_ap_demo_task, 2, 512);
    task_create(wifi_raw_demo_task, 3, 256);
    task_create(wifi_sniffer_task, 4, 512);
    task_create(wifi_wpa3_demo_task, 5, 256);
}
