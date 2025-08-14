/*
 * wifi_stub.h – ESP32-S3 WiFi 802.11 b/g/n + WPA2/WPA3
 */

#ifndef _BLOOD_WIFI_STUB_H
#define _BLOOD_WIFI_STUB_H

#include "kernel/types.h"

/* WiFi modes */
#define WIFI_MODE_OFF 0
#define WIFI_MODE_STA 1
#define WIFI_MODE_AP  2

/* Auth modes */
#define WIFI_AUTH_OPEN    0
#define WIFI_AUTH_WPA     1
#define WIFI_AUTH_WPA2    2
#define WIFI_AUTH_WPA3    3

typedef struct {
    u8 ssid[32];
    u8 ssid_len;
    u8 bssid[6];
    u8 channel;
    s8 rssi;
    u8 auth_mode;
    u8 cipher;
} scan_result_t;

/* Core WiFi functions */
void wifi_init(void);
void wifi_set_mode(u8 mode);
void wifi_set_channel(u8 ch);
void wifi_tx_raw(const u8* pkt, u16 len);
u16 wifi_rx_raw(u8* pkt, u16 max_len);

/* Station mode */
void wifi_scan_start(void);
u8 wifi_get_scan_results(scan_result_t* results, u8 max_results);
u8 wifi_connect(const u8* ssid, u8 ssid_len, const u8* password, u8 pass_len);
u8 wifi_is_connected(void);
void wifi_disconnect(void);

/* Access Point mode */
void wifi_ap_init(const u8* ssid, u8 ssid_len, u8 channel);
void wifi_ap_send_beacon(void);
void wifi_ap_process_frame(const u8* frame, u16 len);
u8 wifi_ap_get_station_count(void);
void wifi_ap_task(void);

/* Security functions */
void wifi_derive_pmk(const u8* ssid, u8 ssid_len, const u8* pass, u8 pass_len);
void wifi_derive_ptk(const u8* anonce, const u8* snonce, const u8* aa, const u8* spa);
u8 wifi_verify_mic(const u8* eapol, u16 len, const u8* mic);
void wifi_encrypt_data(const u8* data, u16 len, u8* out);
void wifi_decrypt_data(const u8* data, u16 len, u8* out);
u8 wifi_wpa3_sae_commit(const u8* peer_mac, const u8* password, u8* commit_msg);
u8 wifi_wpa3_sae_confirm(const u8* commit_msg, u8* confirm_msg);

#endif
