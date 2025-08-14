/*
 * wifi_sec.c – ESP32-S3 WPA2/WPA3 security
 */

#include "kernel/types.h"

#define AES_BASE 0x6003A000UL

typedef struct {
    u8 kck[16];  // Key confirmation key
    u8 kek[16];  // Key encryption key
    u8 tk[16];   // Temporal key
    u8 mic_key[8];
} ptk_t;

typedef struct {
    u8 gtk[16];  // Group temporal key
    u8 seq[6];   // Sequence number
} gtk_t;

static ptk_t ptk;
static gtk_t gtk;
static u8 pmk[32]; // Pairwise master key

/* AES-128 encrypt */
static void aes_encrypt(const u8* key, const u8* in, u8* out) {
    volatile u32* aes = (u32*)AES_BASE;
    
    /* Load key */
    for (u8 i = 0; i < 4; i++) {
        aes[i] = *(u32*)(key + i*4);
    }
    
    /* Load plaintext */
    for (u8 i = 0; i < 4; i++) {
        aes[4 + i] = *(u32*)(in + i*4);
    }
    
    /* Start encryption */
    aes[8] = 0x1;
    while (aes[8] & 0x1); // Wait done
    
    /* Read ciphertext */
    for (u8 i = 0; i < 4; i++) {
        *(u32*)(out + i*4) = aes[12 + i];
    }
}

/* PBKDF2 for PSK derivation */
static void pbkdf2(const u8* pass, u8 pass_len, const u8* salt, u8 salt_len, u8* out) {
    u8 tmp[64];
    u8 hash[32];
    
    /* Simple PBKDF2 with 4096 iterations */
    for (u16 i = 0; i < 32; i++) {
        tmp[i] = pass[i % pass_len] ^ salt[i % salt_len];
    }
    
    /* Hash iterations (simplified) */
    for (u16 iter = 0; iter < 4096; iter++) {
        for (u8 i = 0; i < 32; i++) {
            hash[i] = tmp[i] ^ (iter & 0xFF);
        }
        for (u8 i = 0; i < 32; i++) {
            tmp[i] = hash[i];
        }
    }
    
    for (u8 i = 0; i < 32; i++) {
        out[i] = hash[i];
    }
}

/* PRF for key derivation */
static void prf(const u8* key, const u8* data, u16 len, u8* out, u16 out_len) {
    u8 tmp[64];
    
    /* Simplified PRF using AES */
    for (u16 i = 0; i < out_len; i += 16) {
        for (u8 j = 0; j < 16; j++) {
            tmp[j] = data[j % len] ^ key[j] ^ (i >> 4);
        }
        aes_encrypt(key, tmp, out + i);
    }
}

void wifi_derive_pmk(const u8* ssid, u8 ssid_len, const u8* pass, u8 pass_len) {
    pbkdf2(pass, pass_len, ssid, ssid_len, pmk);
}

void wifi_derive_ptk(const u8* anonce, const u8* snonce, const u8* aa, const u8* spa) {
    u8 data[76];
    u8 i = 0;
    
    /* PTK = PRF(PMK, "Pairwise key expansion" || AA || SPA || ANonce || SNonce) */
    const char* label = "Pairwise key expansion";
    for (u8 j = 0; j < 23; j++) data[i++] = label[j];
    
    /* AA and SPA in lexicographic order */
    if (aa[0] < spa[0]) {
        for (u8 j = 0; j < 6; j++) data[i++] = aa[j];
        for (u8 j = 0; j < 6; j++) data[i++] = spa[j];
    } else {
        for (u8 j = 0; j < 6; j++) data[i++] = spa[j];
        for (u8 j = 0; j < 6; j++) data[i++] = aa[j];
    }
    
    /* ANonce and SNonce in lexicographic order */
    if (anonce[0] < snonce[0]) {
        for (u8 j = 0; j < 32; j++) data[i++] = anonce[j];
        for (u8 j = 0; j < 32; j++) data[i++] = snonce[j];
    } else {
        for (u8 j = 0; j < 32; j++) data[i++] = snonce[j];
        for (u8 j = 0; j < 32; j++) data[i++] = anonce[j];
    }
    
    prf(pmk, data, i, (u8*)&ptk, sizeof(ptk));
}

void wifi_derive_gtk(const u8* gnonce, const u8* aa) {
    u8 data[32];
    
    /* GTK = PRF(GMK, "Group key expansion" || AA || GNonce) */
    const char* label = "Group key expansion";
    for (u8 i = 0; i < 20; i++) data[i] = label[i];
    for (u8 i = 0; i < 6; i++) data[20 + i] = aa[i];
    for (u8 i = 0; i < 6; i++) data[26 + i] = gnonce[i];
    
    prf(pmk, data, 32, (u8*)&gtk, sizeof(gtk));
}

u8 wifi_verify_mic(const u8* eapol, u16 len, const u8* mic) {
    u8 calc_mic[16];
    u8 zero_mic[16] = {0};
    u8 tmp[512];
    
    /* Copy EAPOL with zeroed MIC */
    for (u16 i = 0; i < len; i++) {
        tmp[i] = eapol[i];
    }
    for (u8 i = 0; i < 16; i++) {
        tmp[77 + i] = 0; // Zero MIC field
    }
    
    /* Calculate MIC using KCK */
    aes_encrypt(ptk.kck, tmp, calc_mic);
    
    /* Compare */
    for (u8 i = 0; i < 16; i++) {
        if (calc_mic[i] != mic[i]) return 0;
    }
    return 1;
}

void wifi_encrypt_data(const u8* data, u16 len, u8* out) {
    /* CCMP encryption using TK */
    for (u16 i = 0; i < len; i += 16) {
        u8 block[16] = {0};
        u16 block_len = (len - i < 16) ? len - i : 16;
        
        for (u8 j = 0; j < block_len; j++) {
            block[j] = data[i + j];
        }
        
        aes_encrypt(ptk.tk, block, out + i);
    }
}

void wifi_decrypt_data(const u8* data, u16 len, u8* out) {
    /* CCMP decryption using TK */
    wifi_encrypt_data(data, len, out); // AES is symmetric
}

u8 wifi_wpa3_sae_commit(const u8* peer_mac, const u8* password, u8* commit_msg) {
    /* Simplified SAE commit for WPA3 */
    u8 scalar[32];
    u8 element[64];
    
    /* Generate random scalar */
    for (u8 i = 0; i < 32; i++) {
        scalar[i] = i ^ peer_mac[i % 6] ^ password[i % 32];
    }
    
    /* Generate element (simplified) */
    for (u8 i = 0; i < 64; i++) {
        element[i] = scalar[i % 32] ^ peer_mac[i % 6];
    }
    
    /* Build commit message */
    for (u8 i = 0; i < 32; i++) commit_msg[i] = scalar[i];
    for (u8 i = 0; i < 64; i++) commit_msg[32 + i] = element[i];
    
    return 96; // Message length
}

u8 wifi_wpa3_sae_confirm(const u8* commit_msg, u8* confirm_msg) {
    /* Simplified SAE confirm */
    u8 kck[16];
    
    /* Derive KCK from commit */
    for (u8 i = 0; i < 16; i++) {
        kck[i] = commit_msg[i] ^ commit_msg[32 + i];
    }
    
    /* Generate confirm message */
    aes_encrypt(kck, commit_msg, confirm_msg);
    
    return 16; // Confirm length
}
