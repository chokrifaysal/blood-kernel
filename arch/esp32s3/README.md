# ESP32-S3 Support

**Dual-core RISC-V @ 240 MHz with WiFi 6 + Bluetooth 5**

## Features
- **WiFi 802.11 b/g/n**: 2.4 GHz with 150 Mbps
- **WPA2/WPA3**: Full security protocol support
- **Station Mode**: Scan, connect, authenticate
- **Access Point Mode**: Beacon, association, up to 8 clients
- **Raw Frame TX/RX**: Monitor mode and packet injection
- **WPA3 SAE**: Simultaneous Authentication of Equals
- **AES Hardware**: Accelerated encryption/decryption
- **GPIO Matrix**: Flexible pin routing
- **RTC Slow Memory**: 8 kB persistent storage

## Memory Map
- **Flash**: 8 MB @ 0x42000000 (XIP)
- **SRAM**: 512 kB @ 0x3FC80000
- **RTC Slow**: 8 kB @ 0x50000000
- **WiFi MAC**: 0x60026000
- **AES**: 0x6003A000

## Build & Flash
```bash
make ARCH=esp32s3
make ARCH=esp32s3 flash
```

## WiFi API
```c
/* Station mode */
wifi_scan_start();
wifi_connect(ssid, ssid_len, password, pass_len);
wifi_is_connected();

/* Access Point mode */
wifi_ap_init(ssid, ssid_len, channel);
wifi_ap_task();

/* Raw frames */
wifi_tx_raw(packet, length);
wifi_rx_raw(buffer, max_length);

/* Security */
wifi_derive_pmk(ssid, ssid_len, password, pass_len);
wifi_wpa3_sae_commit(peer_mac, password, commit_msg);
```

## Demo Tasks
- **STA Demo**: Scans and connects to WPA2 networks
- **AP Demo**: Creates access point "ESP32-S3-Demo"
- **Raw Demo**: Sends custom beacon frames
- **Sniffer**: Monitors all channels for packets
- **WPA3 Demo**: Demonstrates SAE handshake

## Performance
- **WiFi**: Up to 150 Mbps 802.11n
- **Range**: ~100m outdoor, ~30m indoor
- **Security**: WPA3-SAE, WPA2-PSK, AES-CCMP
- **Channels**: 1-14 (2.4 GHz)
- **Power**: 20 dBm max TX power
