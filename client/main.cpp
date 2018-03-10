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

void byte_array_to_long(unsigned char* bytes, unsigned long n)
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

void serialize_metadata_to_file(
    char* file_name,
    int blocks_count,
    int enc_oeb_index_size,
    unsigned char* tail_fk, // 32 bytes
    unsigned char* tail_oeb, // 32 bytes
    unsigned char* enc_oeb_index, // enc_oeb_index_size bytes
    unsigned char* iv) // 32 bytes
{
    std::string full_key_name(file_name);
    full_key_name = "tmp_storage/" + full_key_name;
    std::ofstream s(full_key_name);
    s.write(reinterpret_cast<const char*>(&blocks_count), sizeof(blocks_count));
    s.write(reinterpret_cast<const char*>(&enc_oeb_index_size), sizeof(enc_oeb_index_size));
    s.write(reinterpret_cast<const char*>(tail_fk), 32);
    s.write(reinterpret_cast<const char*>(tail_oeb), 32);
    s.write(reinterpret_cast<const char*>(enc_oeb_index), enc_oeb_index_size);
    s.write(reinterpret_cast<const char*>(iv), 32);
    s.close();
}

void deserialize_metadata_file(
    char* file_name,
    int* p_blocks_count,
    int* p_enc_oeb_index_size,
    unsigned char** tail_fk, // 32 bytes
    unsigned char** tail_oeb, // 32 bytes
    unsigned char** enc_oeb_index, // enc_oeb_index_size bytes
    unsigned char** iv)
{
    std::string full_key_name(file_name);
    full_key_name = "tmp_storage/" + full_key_name;
    std::ifstream s(full_key_name);

    s.read(reinterpret_cast<char*>(p_blocks_count), sizeof((*p_blocks_count)));
    s.read(reinterpret_cast<char*>(p_enc_oeb_index_size), sizeof((*p_enc_oeb_index_size)));

    (*tail_fk) = (unsigned char*) malloc(32);
    (*tail_oeb) = (unsigned char*) malloc(32);
    (*enc_oeb_index) = (unsigned char*) malloc(*p_enc_oeb_index_size);
    (*iv) = (unsigned char*) malloc(32);

    s.read(reinterpret_cast<char*>(*tail_fk), 32);
    s.read(reinterpret_cast<char*>(*tail_oeb), 32);
    s.read(reinterpret_cast<char*>(*enc_oeb_index), *p_enc_oeb_index_size);
    s.read(reinterpret_cast<char*>(*iv), 32);

    s.close();
}

void write_to_storage(std::string key, unsigned char* value, size_t size)
{
    std::string full_key_name = "tmp_storage/" + key;
    std::ofstream s(full_key_name);
    s.write(reinterpret_cast<const char*>(value), size);
    s.close();
}

void read_from_storage(std::string key, unsigned char** value, size_t* p_size)
{
    std::string full_key_name = "tmp_storage/" + key;
    std::ifstream s(full_key_name);
    s.seekg(0, std::ifstream::end);
    int file_size = s.tellg();
    (*value) = (unsigned char*) malloc(file_size);
    s.read(reinterpret_cast<char*>(*value), file_size);
    (*p_size) = file_size;
    s.close();
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
        std::string block_name = local_file_name + "." + std::to_string(current_block);
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
            write_to_storage(block_name, enc_enc_block, read_bytes);
        }
        else
        {
            xor_into_array(tail_bi, enc_block_sha, 32);
            write_to_storage(block_name, enc_block, read_bytes);
        }

        printf("Read block of %d\n", read_bytes);

        current_block++;
    }
    in_file.close();

    // encrypt the tail by using the group_key
    unsigned char* enc_tail_fk = (unsigned char*) malloc(32);
    unsigned char* enc_tail_bi = (unsigned char*) malloc(32);
    sgx_aes_encrypt(tail_fk, 32, GK, iv, enc_tail_fk);
    sgx_aes_encrypt(tail_bi, 32, GK, iv, enc_tail_bi);

    // metadata file: blocks count, tail_fk (32), tail_bk (32), enc_oeb_index, iv (32)
    std::string meta_file_name = local_file_name + ".metadata";
    serialize_metadata_to_file(
        (char*) meta_file_name.c_str(),
        number_of_blocks,
        enc_oeb_index_size,
        enc_tail_fk, // 32 bytes
        enc_tail_bi, // 32 bytes
        enc_oeb_index, // enc_oeb_index_size bytes
        iv);

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

int read_file(std::string file_name, unsigned char* GK, std::string local_dest_name)
{
    int number_of_blocks;
    int enc_oeb_index_size;
    unsigned char* enc_tail_fk;
    unsigned char* enc_tail_bi;
    unsigned char* enc_oeb_index;
    unsigned char* iv;

    // read file metadata
    std::string meta_file_name = file_name + ".metadata";
    deserialize_metadata_file(
        (char*) meta_file_name.c_str(),
        &number_of_blocks,
        &enc_oeb_index_size,
        &enc_tail_fk, // 32 bytes
        &enc_tail_bi, // 32 bytes
        &enc_oeb_index, // enc_oeb_index_size bytes
        &iv);

    printf("Reading File ----------- \n");
    printf("Blocks Count %d\n", number_of_blocks);

    // decrypt metadata by GK
    unsigned char* tail_fk = (unsigned char*) malloc(32);
    unsigned char* tail_bi = (unsigned char*) malloc(32);
    sgx_aes_decrypt(enc_tail_fk, 32, GK, iv, tail_fk);
    sgx_aes_decrypt(enc_tail_bi, 32, GK, iv, tail_bi);

    unsigned char* enc_block_sha = (unsigned char*) malloc(32);
    unsigned char* aont_oeb = (unsigned char*) malloc(32);
    memcpy(aont_oeb, tail_bi, 32);

    // get all the blocks
    for(int i=0; i<number_of_blocks; i++)
    {
        unsigned char* enc_block;
        size_t block_size;
        std::string block_name = file_name + "." + std::to_string(i);
        read_from_storage(block_name, &enc_block, &block_size);

        // get hash of encrypted block
        sgx_sha256(enc_block, block_size, enc_block_sha);
        xor_into_array(aont_oeb, enc_block_sha, 32);

        printf("Read Block %d\n", (int) block_size);
    }

    // do a reverse AONT to find out OEB index
    long index_of_oeb;
    byte_array_to_long(aont_oeb, *index_of_oeb)

    // get from storage the over encrypted block
    // ...

    // do a reverse AONT to find out the FK

    // decrypt each block by using FK

    // todo : deallocate

    return 0;
}

int functional_tests()
{
    printf("AONT Client =========== \n");
    srand (time(NULL));

    unsigned char* gk = gen_random_bytestream(32);
    unsigned char* epk = gen_random_bytestream(32);

    write_file("file.dat", gk, rsaPublicKey, strlen(rsaPublicKey));
    read_file("file.dat", gk, "temp.dat");
}

int main()
{
    functional_tests();
}
