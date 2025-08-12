/*
 * wifi_stub.h – minimal ESP32-S3 Wi-Fi MAC stub
 */

#ifndef _BLOOD_WIFI_STUB_H
#define _BLOOD_WIFI_STUB_H

void wifi_init(void);
void wifi_tx_raw(const u8* pkt, u16 len);

#endif
