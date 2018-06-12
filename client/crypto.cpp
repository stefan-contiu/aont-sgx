#include "crypto.h"
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstdlib>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static char _hack_rsaPrivateKey[]="-----BEGIN RSA PRIVATE KEY-----\n"\
"MIIEowIBAAKCAQEAy8Dbv8prpJ/0kKhlGeJYozo2t60EG8L0561g13R29LvMR5hy\n"\
"vGZlGJpmn65+A4xHXInJYiPuKzrKUnApeLZ+vw1HocOAZtWK0z3r26uA8kQYOKX9\n"\
"Qt/DbCdvsF9wF8gRK0ptx9M6R13NvBxvVQApfc9jB9nTzphOgM4JiEYvlV8FLhg9\n"\
"yZovMYd6Wwf3aoXK891VQxTr/kQYoq1Yp+68i6T4nNq7NWC+UNVjQHxNQMQMzU6l\n"\
"WCX8zyg3yH88OAQkUXIXKfQ+NkvYQ1cxaMoVPpY72+eVthKzpMeyHkBn7ciumk5q\n"\
"gLTEJAfWZpe4f4eFZj/Rc8Y8Jj2IS5kVPjUywQIDAQABAoIBADhg1u1Mv1hAAlX8\n"\
"omz1Gn2f4AAW2aos2cM5UDCNw1SYmj+9SRIkaxjRsE/C4o9sw1oxrg1/z6kajV0e\n"\
"N/t008FdlVKHXAIYWF93JMoVvIpMmT8jft6AN/y3NMpivgt2inmmEJZYNioFJKZG\n"\
"X+/vKYvsVISZm2fw8NfnKvAQK55yu+GRWBZGOeS9K+LbYvOwcrjKhHz66m4bedKd\n"\
"gVAix6NE5iwmjNXktSQlJMCjbtdNXg/xo1/G4kG2p/MO1HLcKfe1N5FgBiXj3Qjl\n"\
"vgvjJZkh1as2KTgaPOBqZaP03738VnYg23ISyvfT/teArVGtxrmFP7939EvJFKpF\n"\
"1wTxuDkCgYEA7t0DR37zt+dEJy+5vm7zSmN97VenwQJFWMiulkHGa0yU3lLasxxu\n"\
"m0oUtndIjenIvSx6t3Y+agK2F3EPbb0AZ5wZ1p1IXs4vktgeQwSSBdqcM8LZFDvZ\n"\
"uPboQnJoRdIkd62XnP5ekIEIBAfOp8v2wFpSfE7nNH2u4CpAXNSF9HsCgYEA2l8D\n"\
"JrDE5m9Kkn+J4l+AdGfeBL1igPF3DnuPoV67BpgiaAgI4h25UJzXiDKKoa706S0D\n"\
"4XB74zOLX11MaGPMIdhlG+SgeQfNoC5lE4ZWXNyESJH1SVgRGT9nBC2vtL6bxCVV\n"\
"WBkTeC5D6c/QXcai6yw6OYyNNdp0uznKURe1xvMCgYBVYYcEjWqMuAvyferFGV+5\n"\
"nWqr5gM+yJMFM2bEqupD/HHSLoeiMm2O8KIKvwSeRYzNohKTdZ7FwgZYxr8fGMoG\n"\
"PxQ1VK9DxCvZL4tRpVaU5Rmknud9hg9DQG6xIbgIDR+f79sb8QjYWmcFGc1SyWOA\n"\
"SkjlykZ2yt4xnqi3BfiD9QKBgGqLgRYXmXp1QoVIBRaWUi55nzHg1XbkWZqPXvz1\n"\
"I3uMLv1jLjJlHk3euKqTPmC05HoApKwSHeA0/gOBmg404xyAYJTDcCidTg6hlF96\n"\
"ZBja3xApZuxqM62F6dV4FQqzFX0WWhWp5n301N33r0qR6FumMKJzmVJ1TA8tmzEF\n"\
"yINRAoGBAJqioYs8rK6eXzA8ywYLjqTLu/yQSLBn/4ta36K8DyCoLNlNxSuox+A5\n"\
"w6z2vEfRVQDq4Hm4vBzjdi3QfYLNkTiTqLcvgWZ+eX44ogXtdTDO7c+GeMKWz4XX\n"\
"uJSUVL5+CVjKLjZEJ6Qc2WZLl94xSwL71E41H4YciVnSCQxVc4Jw\n"\
"-----END RSA PRIVATE KEY-----\n";

#ifdef ENABLE_SGX // sgx {
#include <enclave_aont_t.h>
#include <string>
#if defined(__cplusplus) // cxx {
extern "C" {
    int printf(const char *fmt, ...);
}
#endif // } cxx
#endif // } sgx

unsigned char* gen_random_bytestream(int n)
{
    unsigned char* stream = (unsigned char*) malloc(n + 1);

    //int fd = open("/dev/urandom", O_RDONLY);
    //read(fd, stream, n);
    return stream;

    size_t i;
    for (i = 0; i < n; i++)
    {
        stream[i] = (unsigned char) (rand() % 255 + 1);
        //stream[i] = (unsigned char) (11 % 255 + 1);
    }
    stream[n] = 0;
    return stream;
}

void sgx_random(int n, unsigned char b[])
{
    size_t i;
    for (i = 0; i < n; i++)
    {
        //b[i] = (unsigned char) (rand() % 255 + 1);
        b[i] = (unsigned char) (12 % 255 + 1);
    }
}

void xor_into_array(unsigned char* src, unsigned char* array_to_xor, int size)
{
    for(int i=0; i<size; i++)
    {
        src[i] ^= array_to_xor[i];
    }
}

void sgx_aes_encrypt(
    unsigned char* plaintext,
    int plaintext_size,
    unsigned char* key, unsigned char* iv,
    unsigned char* ciphertext)
{
    int len;
    int ciphertext_len;
    EVP_CIPHER_CTX *ctx;
    ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_ctr(), NULL, key, iv);
    EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_size);
    ciphertext_len = len;
    EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
    ciphertext_len += len;
    EVP_CIPHER_CTX_free(ctx);
}

void sgx_aes_decrypt(
    unsigned char* ciphertext,
    int ciphertext_len,
    unsigned char* key, unsigned char* iv,
    unsigned char* plaintext)
{
    EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_len;
    ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_ctr(), NULL, key, iv);
    EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len);
    plaintext_len = len;
    EVP_DecryptFinal_ex(ctx, (plaintext) + len, &len);
    plaintext_len += len;
    EVP_CIPHER_CTX_free(ctx);
}

int rsa_encryption(
    unsigned char* plaintext, int plaintext_length,
    char* key, int key_length,
    unsigned char* ciphertext)
{
    BIO *bio_buffer = NULL;
    RSA *rsa = NULL;

    bio_buffer = BIO_new_mem_buf((void*)key, key_length);
    PEM_read_bio_RSA_PUBKEY(bio_buffer, &rsa, 0, NULL);

    int ciphertext_size = RSA_public_encrypt(
        plaintext_length,
        plaintext,
        ciphertext,
        rsa,
        RSA_PKCS1_PADDING);

    RSA_free(rsa);
    return ciphertext_size;
}

int rsa_decryption(
    unsigned char* ciphertext, int ciphertext_length,
    char* key, int key_length,
    unsigned char* plaintext)
{

    BIO *bio_buffer = NULL;
    RSA *rsa = NULL;
    rsa = RSA_new();


    PEM_read_bio_RSAPrivateKey(bio_buffer, &rsa, 0, NULL);

    int plaintext_length = RSA_private_decrypt(
        ciphertext_length,
        ciphertext,
        plaintext,
        rsa,
        RSA_PKCS1_PADDING);

    RSA_free(rsa);
    return plaintext_length;
}

unsigned char* sgx_sha256(const unsigned char *d,
    size_t n,
    unsigned char *md)
{
    return SHA256(d, n, md);
}
