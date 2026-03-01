#ifndef CRYPTO_H
#define CRYPTO_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

extern bool crypto_init_done;

void Crypto_Init(void);
void Crypto_Cleanup(void);

unsigned char* Crypto_EncryptAES256(const unsigned char *plaintext, int plaintext_len, const unsigned char *key, int *out_len);
unsigned char* Crypto_DecryptAES256(const unsigned char *ciphertext_with_iv, int ciphertext_len, const unsigned char *key, int *out_len);
void Crypto_XOR(unsigned char *data, size_t data_len, const unsigned char *key, size_t key_len);

uint32_t Crypto_CRC32(const unsigned char *data, size_t length);

#endif
