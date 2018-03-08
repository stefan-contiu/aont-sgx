#include <stdio.h>
#include <stdlib.h>
#include "crypto.h"
#include <time.h>
#include <math.h>

#include <fstream>


int BLOCK_SIZE_BYTES = 256 * 1024; // 256 KB

int functional_tests()
{
    printf("AONT Client =========== \n");
    srand (time(NULL));
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
    printf("File size : %d\n", size);
    int number_of_blocks = ceil((double)size / (double)BLOCK_SIZE_BYTES);
    int over_encrypted_block = rand() % number_of_blocks;
    printf("Number of blocks : %d\n", number_of_blocks);
    printf("Over Encrypted Block : %d\n", over_encrypted_block);

    int current_block = 0;
    in_file.seekg(0);
    char* block = new char [BLOCK_SIZE_BYTES];

    unsigned char* tail = (unsigned char*) malloc(32);

    while(in_file)
    {
        in_file.read(block, BLOCK_SIZE_BYTES);
        int read_bytes = in_file.gcount();
        if (current_block == over_encrypted_block)
        {
            printf("OEB ");
        }
        printf("Read block of %d\n", read_bytes);

        // get hash of file
        unsigned char* block_sha;
        sgx_sha256((unsigned char*)block, read_bytes, block_sha);

        // xor each hash with the tail


        current_block++;
    }

    in_file.close();


    // aont the blocks to hide the FK (tail 1)

    // choose randomly a block

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

int main()
{
    functional_tests();
    write_file("file.dat", "", "");
}
