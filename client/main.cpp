#include <stdio.h>
#include <stdlib.h>
#include "crypto.h"
#include <time.h>
#include <math.h>

#include <fstream>
#include <cstring>

char rsaPublicKey[]="-----BEGIN PUBLIC KEY-----\n"\
"MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAy8Dbv8prpJ/0kKhlGeJY\n"\
"ozo2t60EG8L0561g13R29LvMR5hyvGZlGJpmn65+A4xHXInJYiPuKzrKUnApeLZ+\n"\
"vw1HocOAZtWK0z3r26uA8kQYOKX9Qt/DbCdvsF9wF8gRK0ptx9M6R13NvBxvVQAp\n"\
"fc9jB9nTzphOgM4JiEYvlV8FLhg9yZovMYd6Wwf3aoXK891VQxTr/kQYoq1Yp+68\n"\
"i6T4nNq7NWC+UNVjQHxNQMQMzU6lWCX8zyg3yH88OAQkUXIXKfQ+NkvYQ1cxaMoV\n"\
"PpY72+eVthKzpMeyHkBn7ciumk5qgLTEJAfWZpe4f4eFZj/Rc8Y8Jj2IS5kVPjUy\n"\
"wQIDAQAB\n"\
"-----END PUBLIC KEY-----\n";

int BLOCK_SIZE_BYTES = 256 * 1024; // 256 KB

void long_to_byte_array(unsigned long n, unsigned char* bytes, size_t bytes_size)
{
    memset(bytes, 0, bytes_size);
    bytes[0] = (n >> 24) & 0xFF;
    bytes[1] = (n >> 16) & 0xFF;
    bytes[2] = (n >> 8) & 0xFF;
    bytes[3] = n & 0xFF;
}

void byte_array_to_long()
{
    // TODO : ...
}

void xor_into_array(unsigned char* src, unsigned char* array_to_xor, int size)
{
    for(int i=0; i<size; i++)
    {
        src[i] ^= array_to_xor[i];
    }
}

int write_to_storage(std::string block_name, unsigned char* data, size_t size)
{
    return 0;
}

int read_from_storage(std::string block_name)
{
    return 0;
}

int write_file(
        std::string local_file_name,
        unsigned char* GK,
        char* epk, size_t epk_size)
{
    // generate a random AES key : FK
    unsigned char* FK = gen_random_bytestream(32);
    printf("File Key : "); print_hex(FK, 32);

    // read file
    std::ifstream in_file(local_file_name.c_str(), std::ifstream::binary);
    in_file.seekg(0, std::ifstream::end);
    int size = in_file.tellg();

    int number_of_blocks = ceil((double)size / (double)BLOCK_SIZE_BYTES);
    int over_encrypted_block = rand() % number_of_blocks;

    printf("File size : %d\n", size);
    printf("Number of blocks : %d\n", number_of_blocks);
    printf("Over Encrypted Block : %d\n", over_encrypted_block);

    int current_block = 0;
    in_file.seekg(0);
    char* block = new char [BLOCK_SIZE_BYTES];
    unsigned char* enc_block = new unsigned char [BLOCK_SIZE_BYTES];
    unsigned char* enc_enc_block = new unsigned char [BLOCK_SIZE_BYTES];
    unsigned char* iv = gen_random_bytestream(32);
    unsigned char* enc_block_sha = (unsigned char*) malloc(32);
    unsigned char* enc_enc_block_sha = (unsigned char*) malloc(32);;

    // initialize Tail 1 (hides the FK)
    unsigned char* tail_fk = (unsigned char*) malloc(32);
    memcpy(FK, tail_fk, 32);

    // initialize Tail 2 (hides the OEB)
    unsigned char* tail_bi = (unsigned char*) malloc(32);
    long_to_byte_array((unsigned long) over_encrypted_block, tail_bi, 32);
    // encrypt the index of the over-encrypted-block by enclave_public_key
    unsigned char* enc_oeb_index = (unsigned char*) malloc(1024);
    int enc_oeb_index_size = rsa_encryption(tail_bi, 42, epk, epk_size, enc_oeb_index);

    while(in_file)
    {
        in_file.read(block, BLOCK_SIZE_BYTES);
        int read_bytes = in_file.gcount();

        // encrypt block by FK and get its hash
        sgx_aes_encrypt((unsigned char*) block, read_bytes, FK, iv, enc_block);
        sgx_sha256(enc_block, read_bytes, enc_block_sha);

        // xor to Tail 1
        xor_into_array(tail_fk, enc_block_sha, 32);

        // xor to Tail 2
        if (current_block == over_encrypted_block)
        {
            sgx_aes_encrypt((unsigned char*) enc_block, read_bytes, GK, iv, enc_enc_block);
            sgx_sha256((unsigned char*)enc_enc_block, read_bytes, enc_enc_block_sha);
            xor_into_array(tail_bi, enc_enc_block_sha, 32);
        }
        else
        {
            xor_into_array(tail_bi, enc_block_sha, 32);
        }

        printf("Read block of %d\n", read_bytes);
        current_block++;
    }
    in_file.close();

    // encrypt the tail by using the group_key
    unsigned char* enc_tail_fk = (unsigned char*) malloc(32);
    unsigned char* enc_tail_bi = (unsigned char*) malloc(32);
    sgx_aes_encrypt(tail_fk, 32, GK, iv, enc_tail_fk);
    sgx_aes_decrypt(tail_bi, 32, GK, iv, enc_tail_bi);

    // push the data to the storage,
    // metadata file: blocks count, tail_fk, tail_bk, enc_oeb_index, iv

    // blocks files

    free(FK);
    free(block);
    free(enc_block);
    free(enc_enc_block);
    free(enc_block_sha);
    free(enc_enc_block_sha);
    free(tail_fk);
    free(tail_bi);
    free(enc_oeb_index);
    return 0;
}

int read_file()
{
    // read file metadata

    // get all the blocks

    // decrypt the tails by using GK

    // do a reverse AONT to find out OEB index

    // do a reverse AONT to find out the FK

    // decrypt each block by using FK

    return 0;
}

int functional_tests()
{
    printf("AONT Client =========== \n");
    srand (time(NULL));

    unsigned char* gk = gen_random_bytestream(32);
    unsigned char* epk = gen_random_bytestream(32);

    write_file("file.dat", gk, rsaPublicKey, strlen(rsaPublicKey));
    read_file();
}

int main()
{
    functional_tests();
}
