#ifndef CRYPTO_H
#define CRYPTO_H

#include <stdio.h>

#if defined (__cplusplus)
extern "C" {
#endif

unsigned char* gen_random_bytestream(int n);

void xor_into_array(unsigned char* src, unsigned char* array_to_xor, int size);

/* ------- RANDOM -------- */
void sgx_random(int n, unsigned char b[]);

/* ------- AES OPERATIONS ---------- */
void sgx_aes_encrypt(unsigned char* plaintext,
    int plaintext_size,
    unsigned char* key, unsigned char* iv,
    unsigned char* ciphertext);

void sgx_aes_decrypt(unsigned char* ciphertext,
    int ciphertext_len,
    unsigned char* key, unsigned char* iv,
    unsigned char* plaintext);

/* ------- SHA OPERATIONS ---------- */
unsigned char* sgx_sha256(const unsigned char *d,
    size_t n,
    unsigned char *md);

/* ------- RSA OPERATIONS ---------- */
int rsa_encryption(unsigned char* plaintext, int plaintext_length,
    char* key, int key_length,
    unsigned char* ciphertext);

int rsa_decryption(unsigned char* ciphertext, int ciphertext_length,
    char* key, int key_length,
    unsigned char* plaintext);

#if defined (__cplusplus)
}
#endif


// CRYPTO_H
#endif
