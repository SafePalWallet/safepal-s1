
#ifndef WALLET_SECURE_UTIL_H
#define WALLET_SECURE_UTIL_H

#include <stdint.h>
#include <stddef.h>
#include "sha2.h"

#ifdef __cplusplus
extern "C" {
#endif

int aes256_encrypt(const unsigned char *ibuf, unsigned char *obuf, size_t size, const unsigned char *key);

int aes256_decrypt(const unsigned char *ibuf, unsigned char *obuf, size_t size, const unsigned char *key);

void sha256_message(const uint8_t *msg, size_t msglen, const uint8_t *data, size_t len, uint8_t digest[SHA256_DIGEST_LENGTH]);

int aes256_cbc_encrypt(const unsigned char *ibuf, unsigned char *obuf, size_t size, const unsigned char *key);

int aes256_cbc_decrypt(const unsigned char *ibuf, unsigned char *obuf, size_t size, const unsigned char *key);

#ifdef __cplusplus
}
#endif

#endif
