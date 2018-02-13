#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstdlib>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <openssl/evp.h>

unsigned char* gen_random_bytestream(int n)
{
    unsigned char* stream = (unsigned char*) malloc(n + 1);
    size_t i;
    for (i = 0; i < n; i++)
    {
        stream[i] = (unsigned char) (rand() % 255 + 1);
    }
    stream[n] = 0;
    return stream;
}

void aes_encrypt(
    unsigned char* plaintext,
    int plaintext_size,
    unsigned char* key, unsigned char* iv,
    unsigned char* ciphertext)
{
    int len;
    int ciphertext_len;
    EVP_CIPHER_CTX *ctx;
    ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);
    EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_size);
    ciphertext_len = len;
    EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
    ciphertext_len += len;
    EVP_CIPHER_CTX_free(ctx);
}

void aes_decrypt(
    unsigned char* ciphertext,
    int ciphertext_len,
    unsigned char* key, unsigned char* iv,
    unsigned char* plaintext)
{
    EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_len;
    ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);
    EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len);
    plaintext_len = len;
    EVP_DecryptFinal_ex(ctx, (plaintext) + len, &len);
    plaintext_len += len;
    EVP_CIPHER_CTX_free(ctx);
}

int ecall_aont_buffer(const char* in, const int in_size, char* out, int out_size)
{
    // split file in chunks per chunk_size

    // get the hash of each chunk

    // create a random key: FK

    // for each chunk:

    //      encrypt the chunk by using FK

    //      get the hash of the encrypted chunk

    //      xor the hash to the chain of block hashes and FK as genesis

    // pick one of the blocks and overencrypt it with the hash
}

int ecall_deaont_buffer()
{
    //
    return 0;
}


double test_aes_rekey(unsigned char* buffer, int buffer_size)
{
    unsigned char* key = gen_random_bytestream(32);
    unsigned char* iv = gen_random_bytestream(32);
    unsigned char* ciphertext = (unsigned char*) malloc(buffer_size);
    unsigned char* deciphered_plaintext = (unsigned char*) malloc(buffer_size);

    // encrypt, simulate encrypted content
    aes_encrypt(
        buffer, buffer_size,
        key, iv,
        ciphertext);

    // START RE-KEY
    clock_t begin = clock();

    // decrypt using old group key
    aes_decrypt(
        ciphertext, buffer_size,
        key, iv,
        deciphered_plaintext);

    unsigned char* new_key = gen_random_bytestream(32);
    unsigned char* new_iv = gen_random_bytestream(32);
    unsigned char* new_ciphertext = (unsigned char*) malloc(buffer_size);

    // encrypt using new group key
    aes_encrypt(
        deciphered_plaintext, buffer_size,
        new_key, new_iv,
        new_ciphertext);

    // END RE-KEY
    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;

    // free
    /*
    free(key);
    free(iv);
    free(ciphertext);
    free(deciphered_plaintext);
    free(new_key);
    free(new_iv);
    free(new_ciphertext);
*/
    return time_spent;
}

double test_aont_rekey(unsigned char* buffer, int buffer_size, int chunk_size)
{
    // call AONT on package
    // : no importance of the AONT split for this test

    // re-encrypt a chunk size packet
    unsigned char* chunk = gen_random_bytestream(chunk_size);
    return test_aes_rekey(chunk, chunk_size);
}

int main()
{
    // https://blogs.dropbox.com/tech/2014/07/streaming-file-synchronization/
    int test_buff_size = 14 * 1024 * 1024; // 14 MB, as per link above
    unsigned char* buff = gen_random_bytestream(test_buff_size);

    double d = test_aes_rekey(buff, test_buff_size);
    printf("----- AES Re-Key :\n");
    printf("AES 14 MB latency         (s): %f\n", d);
    printf("AES speed              (MB/s): %f\n", 1 / (d / 14));
    printf("(DSN REED) 285 TB latency (h): %f\n", 285 * 1024 * 1024 * (d / 14) / 3600);
    printf("(USENIX)   100 PB latency (d): %f\n", (100 * 1024 / 3600) * 1024 * 1024 * (d / 14) / 24);


    // 4 MB chunk test, same as dropbox
    int chunk_size = 4 * 1024 * 1024;
    d = test_aont_rekey(buff, test_buff_size, chunk_size);
    printf("\n----- AONT Re-Key (4MB chunk):\n");
    printf("AONT 14 MB latency        (s): %f\n", d);
    printf("AONT speed             (MB/s): %f\n", 1 / (d / 14));
    printf("(DSN REED) 285 TB latency (h): %f\n", 285 * 1024 * 1024 * (d / 14) / 3600);
    printf("(USENIX)   100 PB latency (d): %f\n", (100 * 1024 / 3600) * 1024 * 1024 * (d / 14) / 24);

    // 1 MB chunk, Parsec style
    d = test_aont_rekey(buff, test_buff_size, 1 * 1024 * 1024);
    printf("\n----- AONT Re-Key (1MB chunk):\n");
    printf("AONT 14 MB latency        (s): %f\n", d);
    printf("AONT speed             (MB/s): %f\n", 1 / (d / 14));
    printf("(DSN REED) 285 TB latency (h): %f\n", 285 * 1024 * 1024 * (d / 14) / 3600);
    printf("(USENIX)   100 PB latency (d): %f\n", (100 * 1024 / 3600) * 1024 * 1024 * (d / 14) / 24);

    free(buff);
    return 0;
}
