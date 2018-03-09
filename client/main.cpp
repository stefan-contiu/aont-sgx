#include <stdio.h>
#include <stdlib.h>
#include "crypto.h"
#include <time.h>
#include <math.h>

#include <fstream>
#include <cstring>

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

int write_file(
        std::string local_file_name,
        std::string group_key,
        std::string enclave_public_key)
{
    // generate a random AES key : FK
    unsigned char* FK = gen_random_bytestream(32);
    printf("File Key : "); print_hex(FK, 32);

    // read file
    std::ifstream in_file(local_file_name.c_str(), std::ifstream::binary);

    // get size of file
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
    char* enc_block = new char [BLOCK_SIZE_BYTES];
    unsigned char* iv = gen_random_bytestream(32);

    // initialize Tail 1 (hides the FK)
    unsigned char* tail_fk = (unsigned char*) malloc(32);
    memcpy(FK, tail_fk, 32);

    // initialize Tail 2 (hides the OEB)
    unsigned char* tail_bi = (unsigned char*) malloc(32);
    long_to_byte_array((unsigned long) over_encrypted_block, tail_bi, 32);

    while(in_file)
    {
        in_file.read(block, BLOCK_SIZE_BYTES);
        int read_bytes = in_file.gcount();

        // encrypt block by FK
        sgx_aes_encrypt(block, read_bytes, FK, iv, enc_block);

        // xor to Tail 1
        unsigned char* block_sha = (unsigned char*) malloc(32);
        sgx_sha256((unsigned char*)block, read_bytes, block_sha);
        xor_into_array(tail_fk, block_sha, 32);

        // xor to Tail 2
        if (current_block == over_encrypted_block)
        {
            printf("OEB ");
            // over encrypt the block

            // get the hash of the over-encrypted-block
            unsigned char* enc_block_sha = (unsigned char*) malloc(32);;
            sgx_sha256((unsigned char*)block, read_bytes, block_sha);
            for(int i=0; i<32; i++) tail_fk[i] ^= block_sha[i];

            xor_into_array(tail_bi, enc_block_sha, 32);
        }
        else
        {
            xor_into_array(tail_bi, block_sha, 32);
        }
        free(block_sha);


        printf("Read block of %d\n", read_bytes);

        current_block++;
    }

    in_file.close();

    free(FK);
    free(block);
    free(enc_block);
    free(tail_fk);
    free(tail_bi);

    // over-encrypt the block with the GK

    // AONT to hide the index of the over-encrypted-block (tail 2)

    // encrypt the index of the over-encrypted-block by enclave_public_key

    // encrypt the tail by using the group_key

    // psuh the data to the storage
    return 0;
}

int read_file()
{
    return 0;
}

int functional_tests()
{
    printf("AONT Client =========== \n");
    srand (time(NULL));
    write_file("file.dat", "", "");
    read_file();
}

int main()
{
    functional_tests();
}
