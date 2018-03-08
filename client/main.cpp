#include <stdio.h>
#include <stdlib.h>
#include "crypto.h"
#include <time.h>
#include <math.h>

#include <fstream>
#include <cstring>

int BLOCK_SIZE_BYTES = 256 * 1024; // 256 KB

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
    printf("File size : %d\n", size);
    int number_of_blocks = ceil((double)size / (double)BLOCK_SIZE_BYTES);
    int over_encrypted_block = rand() % number_of_blocks;
    printf("Number of blocks : %d\n", number_of_blocks);
    printf("Over Encrypted Block : %d\n", over_encrypted_block);

    int current_block = 0;
    in_file.seekg(0);
    char* block = new char [BLOCK_SIZE_BYTES];

    unsigned char* tail_fk = (unsigned char*) malloc(32);
    memcpy(FK, tail, 32);

    unsigned char* tail_bi = (unsigned char*) malloc(32);
    // copy block index in byte array

    while(in_file)
    {
        in_file.read(block, BLOCK_SIZE_BYTES);
        int read_bytes = in_file.gcount();

        // TODO : symm encrypt the block by using FK, the rest of
        // operations should make use of the enc_block


        // get hash of file and xor it to the tail
        unsigned char* block_sha = (unsigned char*) malloc(32);
        sgx_sha256((unsigned char*)block, read_bytes, block_sha);
        for(int i=0; i<32; i++) tail_fk[i] ^= block_sha[i];

        //
        if (current_block == over_encrypted_block)
        {
            printf("OEB ");
            // over encrypt the block


            unsigned char* enc_block_sha = (unsigned char*) malloc(32);;
            sgx_sha256((unsigned char*)block, read_bytes, block_sha);
            for(int i=0; i<32; i++) tail_fk[i] ^= block_sha[i];

            // encrypt the block and get sha
            // xor it to the tail_bi
        }
        else
        {
            // for the rest of the blocks just use the ciphertext sha
            for(int i=0; i<32; i++) tail_bi[i] ^= block_sha[i];
        }
        free(block_sha);


        printf("Read block of %d\n", read_bytes);

        current_block++;
    }

    in_file.close();

    free(block);
    free(FK);
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
