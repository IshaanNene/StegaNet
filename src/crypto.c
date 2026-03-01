#include "crypto.h"
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool crypto_init_done = false;

void Crypto_Init(void) {
  if (!crypto_init_done) {
    crypto_init_done = true;
  }
}

void Crypto_Cleanup(void) { crypto_init_done = false; }

unsigned char *Crypto_EncryptAES256(const unsigned char *plaintext,
                                    int plaintext_len, const unsigned char *key,
                                    int *out_len) {
  EVP_CIPHER_CTX *ctx = NULL;
  int len;
  int ciphertext_len;
  unsigned char iv[AES_BLOCK_SIZE];

  // Generate random IV
  if (!RAND_bytes(iv, sizeof(iv))) {
    return NULL;
  }

  // Max encrypted size is plaintext_len + block_size (padding) + IV size
  unsigned char *ciphertext =
      malloc(plaintext_len + AES_BLOCK_SIZE + sizeof(iv));
  if (!ciphertext)
    return NULL;

  // Prepend IV
  memcpy(ciphertext, iv, sizeof(iv));

  if (!(ctx = EVP_CIPHER_CTX_new())) {
    free(ciphertext);
    return NULL;
  }

  if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) {
    EVP_CIPHER_CTX_free(ctx);
    free(ciphertext);
    return NULL;
  }

  if (1 != EVP_EncryptUpdate(ctx, ciphertext + sizeof(iv), &len, plaintext,
                             plaintext_len)) {
    EVP_CIPHER_CTX_free(ctx);
    free(ciphertext);
    return NULL;
  }
  ciphertext_len = len;

  if (1 != EVP_EncryptFinal_ex(ctx, ciphertext + sizeof(iv) + len, &len)) {
    EVP_CIPHER_CTX_free(ctx);
    free(ciphertext);
    return NULL;
  }
  ciphertext_len += len;

  EVP_CIPHER_CTX_free(ctx);
  *out_len = ciphertext_len + sizeof(iv);
  return ciphertext;
}

unsigned char *Crypto_DecryptAES256(const unsigned char *ciphertext_with_iv,
                                    int ciphertext_len,
                                    const unsigned char *key, int *out_len) {
  EVP_CIPHER_CTX *ctx = NULL;
  int len;
  int plaintext_len;
  unsigned char iv[AES_BLOCK_SIZE];

  if (ciphertext_len < (int)sizeof(iv)) {
    return NULL; // Too short to even contain IV
  }

  // Extract IV
  memcpy(iv, ciphertext_with_iv, sizeof(iv));

  const unsigned char *actual_ciphertext = ciphertext_with_iv + sizeof(iv);
  int actual_ciphertext_len = ciphertext_len - sizeof(iv);

  unsigned char *plaintext = malloc(actual_ciphertext_len + AES_BLOCK_SIZE);
  if (!plaintext)
    return NULL;

  if (!(ctx = EVP_CIPHER_CTX_new())) {
    free(plaintext);
    return NULL;
  }

  if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) {
    EVP_CIPHER_CTX_free(ctx);
    free(plaintext);
    return NULL;
  }

  if (1 != EVP_DecryptUpdate(ctx, plaintext, &len, actual_ciphertext,
                             actual_ciphertext_len)) {
    EVP_CIPHER_CTX_free(ctx);
    free(plaintext);
    return NULL;
  }
  plaintext_len = len;

  if (1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) {
    EVP_CIPHER_CTX_free(ctx);
    free(plaintext);
    return NULL;
  }
  plaintext_len += len;

  EVP_CIPHER_CTX_free(ctx);

  plaintext[plaintext_len] = '\0';
  *out_len = plaintext_len;
  return plaintext;
}

void Crypto_XOR(unsigned char *data, size_t data_len, const unsigned char *key,
                size_t key_len) {
  if (key_len == 0)
    return;
  for (size_t i = 0; i < data_len; ++i) {
    data[i] ^= key[i % key_len];
  }
}

uint32_t Crypto_CRC32(const unsigned char *data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) crc = (crc >> 1) ^ 0xEDB88320;
            else crc >>= 1;
        }
    }
    return ~crc;
}
