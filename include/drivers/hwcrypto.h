/*
 * hwcrypto.h – x86 hardware cryptography acceleration (AES-NI/SHA-NI/RDRAND)
 */

#ifndef HWCRYPTO_H
#define HWCRYPTO_H

#include "kernel/types.h"

/* Crypto operation types */
#define HWCRYPTO_OP_AES_ENCRYPT		0
#define HWCRYPTO_OP_AES_DECRYPT		1
#define HWCRYPTO_OP_AES_KEYGEN		2
#define HWCRYPTO_OP_SHA1		3
#define HWCRYPTO_OP_SHA256		4
#define HWCRYPTO_OP_PCLMUL		5

/* AES key sizes */
#define AES_KEY_128			128
#define AES_KEY_192			192
#define AES_KEY_256			256

/* Hardware crypto capabilities */
#define HWCRYPTO_CAP_AESNI		(1 << 0)
#define HWCRYPTO_CAP_SHANI		(1 << 1)
#define HWCRYPTO_CAP_PCLMULQDQ		(1 << 2)
#define HWCRYPTO_CAP_RDRAND		(1 << 3)
#define HWCRYPTO_CAP_RDSEED		(1 << 4)
#define HWCRYPTO_CAP_GFNI		(1 << 5)
#define HWCRYPTO_CAP_VAES		(1 << 6)
#define HWCRYPTO_CAP_VPCLMULQDQ		(1 << 7)

typedef struct {
	u32 capabilities;
	u32 aes_ops_count;
	u32 sha_ops_count;
	u32 rng_requests;
	u32 rng_failures;
	u64 total_bytes_encrypted;
	u64 total_bytes_hashed;
	u32 avg_encrypt_cycles;
	u32 avg_hash_cycles;
} hwcrypto_stats_t;

typedef struct {
	u8 key[32];
	u32 key_len;
	u8 rounds;
	u32 expanded_key[60];
} aes_ctx_t;

typedef struct {
	u32 h[8];
	u8 buffer[64];
	u32 buffer_len;
	u64 total_len;
} sha256_ctx_t;

/* Core functions */
void hwcrypto_init(void);

/* Capability detection */
u8 hwcrypto_is_supported(void);
u8 hwcrypto_has_aesni(void);
u8 hwcrypto_has_shani(void);
u8 hwcrypto_has_pclmulqdq(void);
u8 hwcrypto_has_rdrand(void);
u8 hwcrypto_has_rdseed(void);
u32 hwcrypto_get_capabilities(void);

/* AES-NI operations */
u8 hwcrypto_aes_init(aes_ctx_t *ctx, const u8 *key, u32 key_len);
u8 hwcrypto_aes_encrypt_block(const aes_ctx_t *ctx, const u8 *input, u8 *output);
u8 hwcrypto_aes_decrypt_block(const aes_ctx_t *ctx, const u8 *input, u8 *output);
u8 hwcrypto_aes_encrypt_cbc(const aes_ctx_t *ctx, const u8 *iv, 
			    const u8 *input, u8 *output, u32 len);
u8 hwcrypto_aes_decrypt_cbc(const aes_ctx_t *ctx, const u8 *iv,
			    const u8 *input, u8 *output, u32 len);

/* SHA extensions */
u8 hwcrypto_sha256_init(sha256_ctx_t *ctx);
u8 hwcrypto_sha256_update(sha256_ctx_t *ctx, const u8 *data, u32 len);
u8 hwcrypto_sha256_final(sha256_ctx_t *ctx, u8 *hash);
u8 hwcrypto_sha256_oneshot(const u8 *data, u32 len, u8 *hash);

/* Hardware RNG */
u32 hwcrypto_rdrand32(void);
u64 hwcrypto_rdrand64(void);
u32 hwcrypto_rdseed32(void);
u64 hwcrypto_rdseed64(void);
u8 hwcrypto_get_random_bytes(u8 *buffer, u32 len);

/* PCLMULQDQ operations */
u8 hwcrypto_pclmul(u64 a, u64 b, u64 *result_high, u64 *result_low);
u8 hwcrypto_ghash(const u8 *h, const u8 *data, u32 len, u8 *result);

/* Performance monitoring */
const hwcrypto_stats_t* hwcrypto_get_stats(void);
void hwcrypto_clear_stats(void);
u32 hwcrypto_benchmark_aes(u32 iterations);
u32 hwcrypto_benchmark_sha256(u32 iterations);

/* Utilities */
const char* hwcrypto_get_capability_name(u32 cap_bit);
void hwcrypto_print_capabilities(void);
u8 hwcrypto_self_test(void);

#endif