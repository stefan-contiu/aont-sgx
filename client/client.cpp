#include "client.h"

#include "crypto.h"
#include "serialization.h"
#include "storage.h"

#include <fstream>
#include <sstream>
#include <math.h>
#include <tuple>
#include <set>

std::pair<double, double> write_file_aes(
    std::string local_file_name,
    std::string s,
    unsigned char* GK,
    char* epk, size_t epk_size,
    int BLOCK_SIZE_BYTES)
{
    /*
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
        1,
        0,
        (unsigned char*)"", // 32 bytes
        (unsigned char*)"", // 32 bytes
        (unsigned char*)"", // enc_oeb_index_size bytes
        iv);
    clock_t _end = clock();
    storage_time += (double)(_end - _begin) / CLOCKS_PER_SEC;

    return std::make_pair(storage_time, aes_time);
    */
}

std::pair<double, double> write_file(
        std::string local_file_name,
        std::string s,
        unsigned char* GK,
        char* epk, size_t epk_size,
        int BLOCK_SIZE_BYTES,
        int SE_BLOCKS_COUNT)
{
    // generate a random AES key : FK
    unsigned char* FK = gen_random_bytestream(32);
    unsigned char* SK = gen_random_bytestream(32);
    unsigned char* iv = gen_random_bytestream(32);
    //printf("enc FK : "); print_hex(FK, 32);
    //printf("enc SK : "); print_hex(SK, 32);

    int size = s.length();
    std::stringstream in_file(s);

    int number_of_blocks = ceil((double)size / (double)BLOCK_SIZE_BYTES);
    //printf("[write] total blocks : %d. Super encrypted blocsk : %d\n", number_of_blocks, SE_BLOCKS_COUNT);

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

    // initialize Tail 2 (hides the SK)
    unsigned char* tail_sk = (unsigned char*) malloc(32);
    unsigned char* tail_sgx = (unsigned char*) malloc(256);
    memcpy(tail_sk, SK, 32);
    rsa_encryption(SK, 32, epk, epk_size, tail_sgx);

    // initialize tails
    unsigned char* tails_se[SE_BLOCKS_COUNT];
    std::set<int> super_encrypted_blocks_index;
    for(int i=0; i<SE_BLOCKS_COUNT; i++)
    {
        // get a random block, make sure it is not already taken
        int over_encrypted_block;
        do
        {
            over_encrypted_block = rand() % number_of_blocks;
        }
        while(super_encrypted_blocks_index.find(over_encrypted_block) !=
            super_encrypted_blocks_index.end());
        //printf("[write] super encrypt block index : %d\n", over_encrypted_block);
        super_encrypted_blocks_index.insert(over_encrypted_block);

        // initialize the tail_read with it
        tails_se[i] = (unsigned char *) malloc(32);
        unsigned char se_index_bytes[32];
        long_to_byte_array((unsigned long) over_encrypted_block, se_index_bytes, 32);
        sgx_aes_encrypt((unsigned char*) se_index_bytes, 32, SK, iv, tails_se[i]);
    }

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

        //printf("[write] -> read block  : %d\n", current_block); // 256 bytes

        // encrypt block by FK
        {
            clock_t _begin = clock();
            sgx_aes_encrypt((unsigned char*) block, read_bytes, FK, iv, enc_block);
            clock_t _end = clock();
            aes_time += (double)(_end - _begin) / CLOCKS_PER_SEC;
        }

        // chain encrypted block hash for tail_fk
        {
            sgx_sha256(enc_block, read_bytes, enc_block_sha);
            xor_into_array(tail_fk, enc_block_sha, 32);
            //print_hex(enc_block_sha, 32);
        }

        if (super_encrypted_blocks_index.find(current_block) !=
            super_encrypted_blocks_index.end())
        {
            //printf("[write] -> super encrypt this block  : %d\n", current_block); // 256 bytes

            // super encrypt the block
            {
                clock_t _begin = clock();
                sgx_aes_encrypt((unsigned char*) enc_block, read_bytes, GK, iv, enc_enc_block);
                clock_t _end = clock();
                aes_time += (double)(_end - _begin) / CLOCKS_PER_SEC;
            }

            // chain super encrypted block hash in tail_sk
            {
                sgx_sha256(enc_enc_block, read_bytes, enc_enc_block_sha);
                xor_into_array(tail_sk, enc_enc_block_sha, 32);
                //print_hex(enc_enc_block_sha, 32);
            }
        }
        else {
            // for single encrypted blocks, add the hash of single encryption to tail_sk
            xor_into_array(tail_sk, enc_block_sha, 32);
            //print_hex(enc_block_sha, 32);
        }

        // write to storage
        {
            clock_t xbegin = clock();
            if (super_encrypted_blocks_index.find(current_block) !=
                super_encrypted_blocks_index.end())
            {
                write_to_storage(block_name, enc_enc_block, read_bytes);
            }
            else
            {
                write_to_storage(block_name, enc_block, read_bytes);
            }
            clock_t xend = clock();
            storage_time += (double)(xend - xbegin) / CLOCKS_PER_SEC;
        }

        current_block++;
    }

    clock_t begin = clock();

    // encrypt the tail_fk and tail_sk by using GK
    unsigned char* enc_tail_fk = (unsigned char*) malloc(32);
    unsigned char* enc_tail_sk = (unsigned char*) malloc(32);
    sgx_aes_encrypt(tail_fk, 32, GK, iv, enc_tail_fk);
    sgx_aes_encrypt(tail_sk, 32, GK, iv, enc_tail_sk);

    //printf("ENC Tail FK : "); print_hex(tail_fk, 32);
    //printf("ENC GK : "); print_hex(GK, 32);
    //printf("ENC IV : "); print_hex(iv, 32);
    //printf("DEC Tail FK : "); print_hex(enc_tail_fk, 32);

    std::string meta_file_name = local_file_name + ".metadata";

    serialize_metadata_to_file(
        (char*) meta_file_name.c_str(),
        number_of_blocks,
        SE_BLOCKS_COUNT,
        enc_tail_fk, // 32 bytes
        enc_tail_sk, // 32 bytes
        tails_se,
        tail_sgx,
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
    free(tail_sk);

    return std::make_pair(storage_time + meta_time, aes_time);
}

std::pair<double, double> read_file_aes(std::string file_name, unsigned char* GK, std::string local_dest_name,
    int BLOCK_SIZE_BYTES)
{
    /*
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
    */
}


std::pair<double, double> read_file(
    std::string key_name,
    std::string& value,
    unsigned char* GK,
    int BLOCK_SIZE_BYTES)
{
    int number_of_blocks;
    int number_of_super_encrypted_blocks;
    unsigned char* enc_tail_fk;
    unsigned char* enc_tail_sk;
    unsigned char* tails_se[32];
    unsigned char* tail_sgx;    // will not be used by the reader
    unsigned char* iv;

    double storage_time;
    double aes_time;

    clock_t begin = clock();

    // read file metadata
    std::string meta_file_name = key_name + ".metadata";
    deserialize_metadata_file(
        (char*) meta_file_name.c_str(),
        &number_of_blocks,
        &number_of_super_encrypted_blocks,
        &enc_tail_fk, // 32 bytes
        &enc_tail_sk, // 32 bytes
        tails_se,
        &tail_sgx,
        &iv);

    //printf("[read] total blocks : %d, super encrypted blocks : %d\n",
    //    number_of_blocks, number_of_super_encrypted_blocks);

    //printf("Reading File With ----------- \n");
    //printf("Blocks Count %d\n", number_of_blocks);
    //printf("ENC oeb index size %d\n", enc_oeb_index_size);
    //printf("ENC Tail FK : "); print_hex(enc_tail_fk, 32);
    //printf("Enc Tail BI : "); print_hex(enc_tail_bi, 32);
    //printf("Enc Oeb Index : "); print_hex(enc_oeb_index, enc_oeb_index_size);
    //printf("IV : "); print_hex(iv, 32);

    // decrypt metadata by GK
    unsigned char* tail_fk = (unsigned char*) malloc(32);
    unsigned char* tail_sk = (unsigned char*) malloc(32);
    sgx_aes_decrypt(enc_tail_fk, 32, GK, iv, tail_fk);
    sgx_aes_decrypt(enc_tail_sk, 32, GK, iv, tail_sk);

//    printf("ENC GK : "); print_hex(GK, 32);
//    printf("ENC IV : "); print_hex(iv, 32);

    //printf("DEC Tail FK : "); print_hex(tail_fk, 32);


    clock_t end = clock();
    storage_time += (double)(end - begin) / CLOCKS_PER_SEC;

    // first goal : discover SK, perform an AONT on tail_sk
    unsigned char* enc_block_sha = (unsigned char*) malloc(32);
    unsigned char* aont_sk = (unsigned char*) malloc(32);
    unsigned char all_hashes[32] = {0};
    memcpy(aont_sk, tail_sk, 32);

    std::vector<std::tuple<unsigned char*, int>> blocks;
    std::vector<std::tuple<unsigned char*, int>> hashes;

    // get all the blocks
    for(int i=0; i<number_of_blocks; i++)
    {
        unsigned char* enc_block;
        size_t block_size;
        std::string block_name = key_name + "." + std::to_string(i);

        // read block from storage
        {
            clock_t rbegin = clock();
            read_from_storage(block_name, &enc_block, &block_size);
            clock_t rend = clock();
            storage_time += (double)(rend - rbegin) / CLOCKS_PER_SEC;
        }
        //printf("[read] -> block %d downloaded\n", i);

        // get hash of encrypted block
        {
            sgx_sha256(enc_block, block_size, enc_block_sha);
            xor_into_array(aont_sk, enc_block_sha, 32);
            xor_into_array(all_hashes, enc_block_sha, 32);
            //print_hex(enc_block_sha, 32);

            // cache block and hash
            unsigned char* b = (unsigned char*) malloc(block_size);
            memcpy(b, enc_block, block_size);
            unsigned char* h = (unsigned char*) malloc(32);
            memcpy(h, enc_block_sha, 32);

            blocks.push_back(std::make_tuple(b, block_size));
            hashes.push_back(std::make_tuple(h, 32));
            //printf("Added hash : "); print_hex(std::get<0>(hashes[hashes.size() - 1]), 32);
    //    printf("Read Block %d\n", (int) block_size);
        }
    }

    // reveal SK
    unsigned char SK[32];
    memcpy(SK, aont_sk, 32);
    //printf("dec SK : "); print_hex(SK, 32);

    // reveal the indexes of super encrypted blocks
    for (int i=0; i<number_of_super_encrypted_blocks; i++)
    {
        unsigned char se_index_bytes[32];
        sgx_aes_decrypt(tails_se[i], 32, SK, iv, se_index_bytes);
        unsigned long se_index = 0;
        byte_array_to_long(se_index_bytes, &se_index);
        //printf("[read] Super Encrypted Block Index : %d\n", (int)se_index);
        unsigned char dec_block_sha[32];

        // decrypt super-encrypted block
        {
            unsigned char* enc_enc_block = std::get<0>(blocks[se_index]);
            size_t block_size = std::get<1>(blocks[se_index]);
            unsigned char* enc_block = (unsigned char*) malloc(block_size);
            sgx_aes_decrypt(enc_enc_block, block_size, GK, iv, enc_block);
            sgx_sha256(enc_block, block_size, dec_block_sha);

            // reaplce the block in the cache
            std::get<0>(blocks[se_index]) = enc_block;
        }

        // xor old hash out and new hash in
        {
            unsigned char* hash_out = std::get<0>(hashes[se_index]);
            //printf("Hash out : "); print_hex(hash_out, 32);
            //printf("Hash in  : "); print_hex(dec_block_sha, 32);

            xor_into_array(all_hashes, hash_out, 32);
            xor_into_array(all_hashes, dec_block_sha, 32);
        }
    }

    // reveal FK
    unsigned char FK[32];
    {
        memcpy(FK, tail_fk, 32);
        xor_into_array(FK, all_hashes, 32);
        //printf("FK : "); print_hex(FK, 32);
    }

    // decrypt the file using FK
    std::stringstream s;
    for(int i=0; i<blocks.size(); i++)
    {
        int b_size = std::get<1>(blocks[i]);
        unsigned char* block = (unsigned char*) malloc(b_size);

        clock_t _begin = clock();
        sgx_aes_decrypt(std::get<0>(blocks[i]), b_size, FK, iv, block);
        clock_t _end = clock();
        aes_time += (double)(_end - _begin) / CLOCKS_PER_SEC;

        s.write(reinterpret_cast<const char*>(block), b_size);
    }
    value = s.str();

    free(enc_tail_fk);
    free(enc_tail_sk);
    free(iv);
//    free(enc_oeb_index);
    free(tail_fk);
    free(tail_sk);
    free(enc_block_sha);
    free(aont_sk);

    return std::make_pair(storage_time, aes_time);
}
