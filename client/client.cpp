#include "client.h"

#include "crypto.h"
#include "serialization.h"
#include "storage.h"

#include <fstream>
#include <sstream>
#include <math.h>
#include <tuple>

std::pair<double, double> write_file_aes(
    std::string local_file_name,
    std::string s,
    unsigned char* GK,
    char* epk, size_t epk_size,
    int BLOCK_SIZE_BYTES)
{
    double aes_time = 0;
    double storage_time = 0;

    std::stringstream in_file(s);
    int size = s.length();

    // read file
    //std::ifstream in_file(local_file_name.c_str(), std::ifstream::binary);
    //in_file.seekg(0, std::ifstream::end);
    //int size = in_file.tellg();

    int number_of_blocks = ceil((double)size / (double)BLOCK_SIZE_BYTES);
    int over_encrypted_block = rand() % number_of_blocks;

    //printf("File size : %d\n", size);
    //printf("Number of blocks : %d\n", number_of_blocks);
    //printf("Over Encrypted Block : %d\n", over_encrypted_block);

    int current_block = 0;
    in_file.seekg(0);
    char* block = new char [BLOCK_SIZE_BYTES];
    unsigned char* enc_block = new unsigned char [BLOCK_SIZE_BYTES];
    unsigned char* iv = gen_random_bytestream(32);

    while(in_file)
    {
        std::string block_name = local_file_name + "." + std::to_string(current_block);
        //printf("Processing %s\n", block_name.c_str());

        in_file.read(block, BLOCK_SIZE_BYTES);
        int read_bytes = in_file.gcount();
        if (read_bytes == 0)
        {
            break;
        }

        clock_t _begin = clock();
        // encrypt block by FK
        sgx_aes_encrypt((unsigned char*) block, read_bytes, GK, iv, enc_block);
        clock_t _end = clock();
        aes_time += (double)(_end - _begin) / CLOCKS_PER_SEC;

        clock_t begin = clock();
        write_to_storage(block_name, enc_block, read_bytes);
        clock_t end = clock();
        storage_time += (double)(end - begin) / CLOCKS_PER_SEC;

        current_block++;
    }
    //in_file.close();

    clock_t _begin = clock();
    std::string meta_file_name = local_file_name + ".metadata";
    serialize_metadata_to_file(
        (char*) meta_file_name.c_str(),
        number_of_blocks,
        0,
        (unsigned char*)"", // 32 bytes
        (unsigned char*)"", // 32 bytes
        (unsigned char*)"", // enc_oeb_index_size bytes
        iv);
    clock_t _end = clock();
    storage_time += (double)(_end - _begin) / CLOCKS_PER_SEC;

    return std::make_pair(storage_time, aes_time);
}

std::pair<double, double> write_file(
        std::string local_file_name,
        std::string s,
        unsigned char* GK,
        char* epk, size_t epk_size,
        int BLOCK_SIZE_BYTES)
{
    // generate a random AES key : FK
    unsigned char* FK = gen_random_bytestream(32);
    unsigned char* iv = gen_random_bytestream(32);

    //print_hex(FK, 32);
    //printf("File Key : "); print_hex(FK, 32);

    int size = s.length();
    std::stringstream in_file(s);
    // read file
    //std::ifstream in_file(local_file_name.c_str(), std::ifstream::binary);
    //in_file.seekg(0, std::ifstream::end);
    //int size = in_file.tellg();

    int number_of_blocks = ceil((double)size / (double)BLOCK_SIZE_BYTES);
    int over_encrypted_block = rand() % number_of_blocks;

    //printf("File size : %d\n", size);
    //printf("Number of blocks : %d\n", number_of_blocks);
    //printf("Over Encrypted Block : %d\n", over_encrypted_block);

    int current_block = 0;
    in_file.seekg(0);
    char* block = new char [BLOCK_SIZE_BYTES];
    unsigned char* enc_block = new unsigned char [BLOCK_SIZE_BYTES];
    unsigned char* enc_enc_block = new unsigned char [BLOCK_SIZE_BYTES];
    unsigned char* enc_block_sha = (unsigned char*) malloc(32);
    unsigned char* enc_enc_block_sha = (unsigned char*) malloc(32);;

    // initialize Tail 1 (hides the FK)
    unsigned char* tail_fk = (unsigned char*) malloc(32);
    memcpy(tail_fk, FK, 32);

    // initialize Tail 2 (hides the OEB)
    unsigned char* tail_bi = (unsigned char*) malloc(32);
    long_to_byte_array((unsigned long) over_encrypted_block, tail_bi, 32);

    // encrypt the index of the over-encrypted-block by enclave_public_key
    unsigned char* enc_oeb_index = (unsigned char*) malloc(1024);
    int enc_oeb_index_size = rsa_encryption(tail_bi, 42, epk, epk_size, enc_oeb_index);

    double storage_time = 0;
    double aes_time = 0;

    while(in_file)
    {
        std::string block_name = local_file_name + "." + std::to_string(current_block);
        in_file.read(block, BLOCK_SIZE_BYTES);
        int read_bytes = in_file.gcount();
        if (read_bytes == 0)
        {
            break;
        }

        clock_t _begin = clock();
        // encrypt block by FK and get its hash
        sgx_aes_encrypt((unsigned char*) block, read_bytes, FK, iv, enc_block);
        clock_t _end = clock();
        aes_time += (double)(_end - _begin) / CLOCKS_PER_SEC;

        sgx_sha256(enc_block, read_bytes, enc_block_sha);
        // xor to Tail 1
        xor_into_array(tail_fk, enc_block_sha, 32);

        // xor to Tail 2
        if (current_block == over_encrypted_block)
        {
            clock_t _begin = clock();
            sgx_aes_encrypt((unsigned char*) enc_block, read_bytes, GK, iv, enc_enc_block);
            clock_t _end = clock();
            aes_time += (double)(_end - _begin) / CLOCKS_PER_SEC;

            sgx_sha256((unsigned char*)enc_enc_block, read_bytes, enc_enc_block_sha);
            xor_into_array(tail_bi, enc_enc_block_sha, 32);
            //write_to_storage(block_name, enc_enc_block, read_bytes);
        }
        else
        {
            xor_into_array(tail_bi, enc_block_sha, 32);
            //write_to_storage(block_name, enc_block, read_bytes);
        }

        //printf("Read block of %d\n", read_bytes);
        clock_t xbegin = clock();
        if (current_block == over_encrypted_block)
        {
            write_to_storage(block_name, enc_enc_block, read_bytes);
        }
        else
        {
            write_to_storage(block_name, enc_block, read_bytes);
        }
        clock_t xend = clock();
        storage_time += (double)(xend - xbegin) / CLOCKS_PER_SEC;


        current_block++;
    }
    //in_file.close();

    clock_t begin = clock();

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

    clock_t end = clock();
    double meta_time = (double)(end - begin) / CLOCKS_PER_SEC;

    free(FK);
    free(block);
    free(enc_block);
    free(enc_enc_block);
    free(enc_block_sha);
    free(enc_enc_block_sha);
    free(tail_fk);
    free(tail_bi);
    free(enc_oeb_index);

    return std::make_pair(storage_time + meta_time, aes_time);
}

std::pair<double, double> read_file_aes(std::string file_name, unsigned char* GK, std::string local_dest_name,
    int BLOCK_SIZE_BYTES)
{
    int number_of_blocks;
    int enc_oeb_index_size;
    unsigned char* enc_tail_fk;
    unsigned char* enc_tail_bi;
    unsigned char* enc_oeb_index;
    unsigned char* iv;

    double storage_time;
    double aes_time;

    clock_t begin = clock();

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
    clock_t end = clock();
    storage_time += (double)(end - begin) / CLOCKS_PER_SEC;

    // get all the blocks
    //std::ofstream s(local_dest_name);
    std::stringstream s;
    for(int i=0; i<number_of_blocks; i++)
    {
        unsigned char* enc_block;
        size_t block_size;
        std::string block_name = file_name + "." + std::to_string(i);

        clock_t begin = clock();
        read_from_storage(block_name, &enc_block, &block_size);
        clock_t end = clock();
        storage_time += (double)(end - begin) / CLOCKS_PER_SEC;

        unsigned char* block = (unsigned char*) malloc(block_size);
        clock_t _begin = clock();
        sgx_aes_decrypt(enc_block, block_size, GK, iv, block);
        clock_t _end = clock();
        aes_time += (double)(_end - _begin) / CLOCKS_PER_SEC;

        s.write(reinterpret_cast<const char*>(block), block_size);
    }
    //s.close();

    return std::make_pair(storage_time, aes_time);
}

std::pair<double, double> read_file(std::string file_name, unsigned char* GK, std::string local_dest_name,
    int BLOCK_SIZE_BYTES)
{
    int number_of_blocks;
    int enc_oeb_index_size;
    unsigned char* enc_tail_fk;
    unsigned char* enc_tail_bi;
    unsigned char* enc_oeb_index;
    unsigned char* iv;

    double storage_time;
    double aes_time;

    clock_t begin = clock();

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

    //printf("Reading File With ----------- \n");
    //printf("Blocks Count %d\n", number_of_blocks);
    //printf("ENC oeb index size %d\n", enc_oeb_index_size);
    //printf("Enc Tail FK : "); print_hex(enc_tail_fk, 32);
    //printf("Enc Tail BI : "); print_hex(enc_tail_bi, 32);
    //printf("Enc Oeb Index : "); print_hex(enc_oeb_index, enc_oeb_index_size);
    //printf("IV : "); print_hex(iv, 32);

    // decrypt metadata by GK
    unsigned char* tail_fk = (unsigned char*) malloc(32);
    unsigned char* tail_bi = (unsigned char*) malloc(32);
    sgx_aes_decrypt(enc_tail_fk, 32, GK, iv, tail_fk);
    sgx_aes_decrypt(enc_tail_bi, 32, GK, iv, tail_bi);

    clock_t end = clock();
    storage_time += (double)(end - begin) / CLOCKS_PER_SEC;


    unsigned char* enc_block_sha = (unsigned char*) malloc(32);
    unsigned char* aont_oeb = (unsigned char*) malloc(32);
    memcpy(aont_oeb, tail_bi, 32);


    std::vector<std::tuple<unsigned char*, int>> blocks;

    // get all the blocks
    for(int i=0; i<number_of_blocks; i++)
    {
    //    printf("Reading %d\n", i);
        unsigned char* enc_block;
        size_t block_size;
        std::string block_name = file_name + "." + std::to_string(i);

        clock_t rbegin = clock();
        read_from_storage(block_name, &enc_block, &block_size);
        clock_t rend = clock();
        storage_time += (double)(rend - rbegin) / CLOCKS_PER_SEC;

        // get hash of encrypted block
        sgx_sha256(enc_block, block_size, enc_block_sha);
        xor_into_array(aont_oeb, enc_block_sha, 32);
        blocks.push_back(std::make_tuple(enc_block, block_size));
    //    printf("Read Block %d\n", (int) block_size);
    }

    // do a reverse AONT to find out OEB index
    unsigned long index_of_oeb;
    byte_array_to_long(aont_oeb, &index_of_oeb);
    //printf("Over Encrypted Block Index : %d\n", (int)index_of_oeb);

    // fk = t_bi ^ t_fk ^ x4 ^ h4 ^ bi
    xor_into_array(tail_fk, tail_bi, 32);
    xor_into_array(tail_fk, aont_oeb, 32);

    unsigned char* enc_block = std::get<0>(blocks[index_of_oeb]);
    size_t block_size = std::get<1>(blocks[index_of_oeb]);
    sgx_sha256(enc_block, block_size, enc_block_sha);
    xor_into_array(tail_fk, enc_block_sha, 32);

    // decrypt over encrypted block by GK
    unsigned char* enc_enc_block = (unsigned char*) malloc(block_size);
    clock_t _begin = clock();
    sgx_aes_decrypt(enc_block, block_size, GK, iv, enc_enc_block);
    clock_t _end = clock();
    aes_time += (double)(_end - _begin) / CLOCKS_PER_SEC;

    unsigned char* enc_enc_block_sha = (unsigned char*) malloc(32);
    sgx_sha256(enc_enc_block, block_size, enc_enc_block_sha);
    xor_into_array(tail_fk, enc_enc_block_sha, 32);

    // get FK and decrypt file
    //print_hex(tail_fk, 32);
    //std::ofstream s(local_dest_name);
    std::stringstream s;
    for(int i=0; i<blocks.size(); i++)
    {
        int b_size = std::get<1>(blocks[i]);
        unsigned char* block = (unsigned char*) malloc(b_size);

        clock_t _begin = clock();
        if (i != index_of_oeb)
        {
            sgx_aes_decrypt(std::get<0>(blocks[i]), b_size, tail_fk, iv, block);
        }
        else
        {
            sgx_aes_decrypt(enc_enc_block, block_size, tail_fk, iv, block);
        }
        clock_t _end = clock();
        aes_time += (double)(_end - _begin) / CLOCKS_PER_SEC;

        s.write(reinterpret_cast<const char*>(block), b_size);
    }
    //s.close();

    //printf("del fk\n");
    free(enc_tail_fk);
    //printf("del bi\n");
    free(enc_tail_bi);
    //printf("del iv\n");
    free(iv);
    //printf("del oeb\n");
    free(enc_oeb_index);
    //printf("del fk\n");
    free(tail_fk);
    //printf("del bi\n");
    free(tail_bi);
    //printf("enc sha\n");
    free(enc_block_sha);
    //printf("aont oeb\n");
    free(aont_oeb);

    return std::make_pair(storage_time, aes_time);
}
