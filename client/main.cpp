#include <stdio.h>
#include <stdlib.h>
#include "crypto.h"
#include <time.h>
#include <math.h>

#include <fstream>
#include <cstring>
#include <vector>
#include <tuple>
#include <experimental/filesystem>

#include <iostream>
namespace fs = std::experimental::filesystem::v1;

char rsaPublicKey[]="-----BEGIN PUBLIC KEY-----\n"\
"MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAy8Dbv8prpJ/0kKhlGeJY\n"\
"ozo2t60EG8L0561g13R29LvMR5hyvGZlGJpmn65+A4xHXInJYiPuKzrKUnApeLZ+\n"\
"vw1HocOAZtWK0z3r26uA8kQYOKX9Qt/DbCdvsF9wF8gRK0ptx9M6R13NvBxvVQAp\n"\
"fc9jB9nTzphOgM4JiEYvlV8FLhg9yZovMYd6Wwf3aoXK891VQxTr/kQYoq1Yp+68\n"\
"i6T4nNq7NWC+UNVjQHxNQMQMzU6lWCX8zyg3yH88OAQkUXIXKfQ+NkvYQ1cxaMoV\n"\
"PpY72+eVthKzpMeyHkBn7ciumk5qgLTEJAfWZpe4f4eFZj/Rc8Y8Jj2IS5kVPjUy\n"\
"wQIDAQAB\n"\
"-----END PUBLIC KEY-----\n";

char rsaPrivateKey[]="-----BEGIN RSA PRIVATE KEY-----\n"\
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


int BLOCK_SIZE_BYTES = 256 * 1024; // 256 KB

void long_to_byte_array(unsigned long n, unsigned char* bytes, size_t bytes_size)
{
    memset(bytes, 0, bytes_size);
    bytes[0] = (n >> 24) & 0xFF;
    bytes[1] = (n >> 16) & 0xFF;
    bytes[2] = (n >> 8) & 0xFF;
    bytes[3] = n & 0xFF;
}

void byte_array_to_long(unsigned char* bytes, unsigned long* n)
{
    *n = ((bytes[0] << 24)
         +(bytes[1] << 16)
         +(bytes[2] << 8)
         +(bytes[3]));
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

void deserialize_metadata_stream(
    char* inputStream,
    size_t inputStreamSize,
    int* p_blocks_count,
    int* p_enc_oeb_index_size,
    unsigned char** tail_fk, // 32 bytes
    unsigned char** tail_oeb, // 32 bytes
    unsigned char** enc_oeb_index, // enc_oeb_index_size bytes
    unsigned char** iv)
{
    std::string ss(inputStream, inputStreamSize);
    std::stringstream s(ss);
    s.read(reinterpret_cast<char*>(p_blocks_count), sizeof((*p_blocks_count)));
    printf("BLOCKS COUNT %d\n", *p_blocks_count);
    s.read(reinterpret_cast<char*>(p_enc_oeb_index_size), sizeof((*p_enc_oeb_index_size)));

    (*tail_fk) = (unsigned char*) malloc(32);
    (*tail_oeb) = (unsigned char*) malloc(32);
    (*enc_oeb_index) = (unsigned char*) malloc(*p_enc_oeb_index_size);
    (*iv) = (unsigned char*) malloc(32);

    s.read(reinterpret_cast<char*>(*tail_fk), 32);
    s.read(reinterpret_cast<char*>(*tail_oeb), 32);
    s.read(reinterpret_cast<char*>(*enc_oeb_index), *p_enc_oeb_index_size);
    s.read(reinterpret_cast<char*>(*iv), 32);
}

void serialize_metadata_to_stream(
    unsigned char** meta_stream,
    size_t* p_meta_stream_size,
    int blocks_count,
    int enc_oeb_index_size,
    unsigned char* tail_fk, // 32 bytes
    unsigned char* tail_oeb, // 32 bytes
    unsigned char* enc_oeb_index, // enc_oeb_index_size bytes
    unsigned char* iv
)
{
    std::stringstream s;
    s.write(reinterpret_cast<const char*>(&blocks_count), sizeof(blocks_count));
    s.write(reinterpret_cast<const char*>(&enc_oeb_index_size), sizeof(enc_oeb_index_size));
    s.write(reinterpret_cast<const char*>(tail_fk), 32);
    s.write(reinterpret_cast<const char*>(tail_oeb), 32);
    s.write(reinterpret_cast<const char*>(enc_oeb_index), enc_oeb_index_size);
    s.write(reinterpret_cast<const char*>(iv), 32);
    *p_meta_stream_size = s.str().size();
    *meta_stream = (unsigned char*) malloc(*p_meta_stream_size);
    memcpy(*meta_stream, s.str().c_str(), *p_meta_stream_size);
    print_hex(*meta_stream, 64);
    //*meta_stream = (unsigned char*) s.str().c_str();

    printf(">>>>>>>>>>>> The metadata stream is %d long\n", *p_meta_stream_size);
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
    s.seekg(0);
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
    memcpy(tail_fk, FK, 32);

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

    std::vector<std::tuple<unsigned char*, int>> blocks;

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
        blocks.push_back(std::make_tuple(enc_block, block_size));

        printf("Read Block %d\n", (int) block_size);
    }

    // do a reverse AONT to find out OEB index
    unsigned long index_of_oeb;
    byte_array_to_long(aont_oeb, &index_of_oeb);
    printf("Over Encrypted Block Index : %d\n", (int)index_of_oeb);

    // fk = t_bi ^ t_fk ^ x4 ^ h4 ^ bi
    xor_into_array(tail_fk, tail_bi, 32);
    xor_into_array(tail_fk, aont_oeb, 32);

    unsigned char* enc_block = std::get<0>(blocks[index_of_oeb]);
    size_t block_size = std::get<1>(blocks[index_of_oeb]);
    sgx_sha256(enc_block, block_size, enc_block_sha);
    xor_into_array(tail_fk, enc_block_sha, 32);

    // decrypt over encrypted block by GK
    unsigned char* enc_enc_block = (unsigned char*) malloc(block_size);
    sgx_aes_decrypt(enc_block, block_size, GK, iv, enc_enc_block);

    unsigned char* enc_enc_block_sha = (unsigned char*) malloc(32);
    sgx_sha256(enc_enc_block, block_size, enc_enc_block_sha);
    xor_into_array(tail_fk, enc_enc_block_sha, 32);

    // get FK and decrypt file
    print_hex(tail_fk, 32);
    std::ofstream s(local_dest_name);
    for(int i=0; i<blocks.size(); i++)
    {
        int b_size = std::get<1>(blocks[i]);
        unsigned char* block = (unsigned char*) malloc(b_size);

        if (i != index_of_oeb)
        {
            sgx_aes_decrypt(std::get<0>(blocks[i]), b_size, tail_fk, iv, block);
        }
        else
        {
            sgx_aes_decrypt(enc_enc_block, block_size, tail_fk, iv, block);
        }
        s.write(reinterpret_cast<const char*>(block), b_size);
    }
    s.close();

    free(enc_tail_fk);
    free(enc_tail_bi);
    free(enc_oeb_index);
    free(iv);
    return 0;
}


void get_all_metadata_keys(std::vector<std::string>& metadata_keys)
{
    metadata_keys.clear();
    std::string path = "tmp_storage/";
    for (auto & p : fs::directory_iterator(path))
        if (p.path().string().find(".metadata") != std::string::npos)
        {
            std::string s = p.path().string();
            s.replace(0, path.size(), "");
            metadata_keys.push_back(s);
        }
}

void ocall_get_block(char* blockName, unsigned char** data, size_t* p_size)
{
    std::string key(blockName);
    read_from_storage(key, data, p_size);
}

void ocall_put_block(char* blockName, unsigned char* data, size_t size)
{
    std::string key(blockName);
    write_to_storage(key, data, size);
}

void ecall_worker_re_key(unsigned char* old_gk, unsigned char* new_gk,
    char* key,
    char* metadata, size_t meta_size)
{
    int blocks_count;
    int enc_oeb_index_size;
    unsigned char* tail_fk;
    unsigned char* tail_bi;
    unsigned char* enc_oeb_index;
    unsigned char* iv;

    // deserialize metadata
    deserialize_metadata_stream(
        metadata,
        meta_size,
        &blocks_count,
        &enc_oeb_index_size,
        &tail_fk, // 32 bytes
        &tail_bi, // 32 bytes
        &enc_oeb_index, // enc_oeb_index_size bytes
        &iv);

    printf("SGX says %d blocks\n", blocks_count);

    // decrypt OEB index
    long unsigned oeb;
    unsigned char* dec_oeb_index = (unsigned char*) malloc(64);
    rsa_decryption(enc_oeb_index, enc_oeb_index_size,
        rsaPrivateKey, strlen(rsaPrivateKey),
        dec_oeb_index);
    byte_array_to_long(dec_oeb_index, &oeb);
    printf("SGX says OEB %d\n", (int)oeb);

    // bring the over-encrypted block in the enclave (OCALL)
    std::string blockName = std::string(key);
    // strip .metadata suffix and add block index
    blockName.replace(blockName.size() - 9, 9, "");
    blockName = blockName + "." + std::to_string(oeb);
    std::cout << "fetch " << blockName << "\n";

    unsigned char* enc_block;
    size_t block_size;
    ocall_get_block((char*)blockName.c_str(), &enc_block, &block_size);

    // re-encrypt it
    printf("SGX says Returned block is %d in size.\n", (int)block_size);

    // decrypt it by using old_gk
    unsigned char* dec_enc_block = (unsigned char*) malloc(block_size);
    sgx_aes_decrypt(enc_block, block_size, old_gk, iv, dec_enc_block);

    // encrypt it by using new_gk
    unsigned char* enc_enc_block = (unsigned char*) malloc(block_size);
    sgx_aes_encrypt(dec_enc_block, block_size, new_gk, iv, enc_enc_block);

    // push it back to the storage (OCALL)
    ocall_put_block((char*)blockName.c_str(), enc_enc_block, block_size);

    // encrypt the tail by using the group_key
    unsigned char* dec_tail_fk = (unsigned char*) malloc(32);
    unsigned char* dec_tail_bi = (unsigned char*) malloc(32);
    sgx_aes_decrypt(tail_fk, 32, old_gk, iv, dec_tail_fk);
    sgx_aes_decrypt(tail_bi, 32, old_gk, iv, dec_tail_bi);

    // adjust the tail_bi, xor with the old and new block hash
    unsigned char* enc_block_sha = (unsigned char*) malloc(32);
    sgx_sha256(enc_block, block_size, enc_block_sha);
    xor_into_array(dec_tail_bi, enc_block_sha, 32);

    unsigned char* enc_enc_block_sha = (unsigned char*) malloc(32);
    sgx_sha256(enc_enc_block, block_size, enc_enc_block_sha);
    xor_into_array(dec_tail_bi, enc_enc_block_sha, 32);

    unsigned char* new_tail_fk = (unsigned char*) malloc(32);
    unsigned char* new_tail_bi = (unsigned char*) malloc(32);
    sgx_aes_encrypt(dec_tail_fk, 32, new_gk, iv, new_tail_fk);
    sgx_aes_encrypt(dec_tail_bi, 32, new_gk, iv, new_tail_bi);

    // metadata file: blocks count, tail_fk (32), tail_bk (32), enc_oeb_index, iv (32)
    std::string meta_file_name = blockName + ".metadata";
    unsigned char* meta_stream;
    size_t meta_stream_size;
    printf("Blocks Count %d\n", blocks_count);
    serialize_metadata_to_stream(
        &meta_stream,
        &meta_stream_size,
        blocks_count,
        enc_oeb_index_size,
        new_tail_fk, // 32 bytes
        new_tail_bi, // 32 bytes
        enc_oeb_index, // enc_oeb_index_size bytes
        iv);

    // push back metadata to the storage
    ocall_put_block(key, meta_stream, meta_stream_size);

    // clear up stuff
    free(dec_enc_block);
    free(enc_enc_block);
    free(tail_fk);
    free(tail_bi);
    free(enc_oeb_index);
    free(iv);
    free(dec_tail_fk);
    free(dec_tail_bi);
    free(new_tail_fk);
    free(new_tail_bi);
    free(meta_stream);
}

void re_key(unsigned char* old_gk, unsigned char* new_gk)
{
    std::vector<std::string> metadata;
    get_all_metadata_keys(metadata);

    // read all metadata from files
    for(int i=0; i<metadata.size(); i++)
    {
        printf("Processing [%s]\n", metadata[i].c_str());

        unsigned char* meta_stream;
        size_t meta_size;
        read_from_storage(metadata[i], &meta_stream, &meta_size);
        //print_hex(meta_stream, 64);

        // go to SGX
        ecall_worker_re_key(old_gk, new_gk,
            (char*) metadata[i].c_str(),
            (char*) meta_stream, meta_size);
    }
}

int functional_tests()
{
    printf("AONT Client =========== \n");
    srand (time(NULL));

    unsigned char* gk = gen_random_bytestream(32);

    write_file("file.dat", gk, rsaPublicKey, strlen(rsaPublicKey));
    read_file("file.dat", gk, "temp.dat");

    unsigned char* new_gk = gen_random_bytestream(32);
    re_key(gk, new_gk);
    read_file("file.dat", new_gk, "temp_rekeyed.dat");
}

int main()
{
    functional_tests();
}
