//#include "redis.h"
#include "rest.h"
#include "serialization.h"

#include <string>

#include <fstream>
#include <sstream>


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

void pack32(uint32_t val,uint8_t *dest)
{
        dest[0] = (val & 0xff000000) >> 24;
        dest[1] = (val & 0x00ff0000) >> 16;
        dest[2] = (val & 0x0000ff00) >>  8;
        dest[3] = (val & 0x000000ff)      ;
}

uint32_t unpack32(uint8_t *src)
{
        uint32_t val;

        val  = src[0] << 24;
        val |= src[1] << 16;
        val |= src[2] <<  8;
        val |= src[3]      ;

        return val;
}

void deserialize_metadata_stream(
    unsigned char* inputStream,
    int inputStreamSize,
    int* p_blocks_count,
    int* p_se_blocks_count,
    unsigned char** p_tail_fk, // 32 bytes
    unsigned char** p_tail_sk, // 32 bytes
    unsigned char* tails_se[32], // enc_oeb_index_size bytes
    unsigned char** p_tail_sgx,
    unsigned char** p_iv)
{

    *p_blocks_count = unpack32(inputStream);
    inputStream += 4;
    *p_se_blocks_count = unpack32(inputStream);
    inputStream += 4;

    (*p_tail_fk) = (unsigned char*) malloc(32);
    (*p_tail_sk) = (unsigned char*) malloc(32);
    (*p_tail_sgx) = (unsigned char*) malloc(256);
    (*p_iv) = (unsigned char*) malloc(32);

    memcpy(*p_tail_fk, inputStream, 32);
    inputStream += 32;

    memcpy(*p_tail_sk, inputStream, 32);
    inputStream += 32;

    for(int i=0; i<*p_se_blocks_count; i++)
    {
        tails_se[i] = (unsigned char*) malloc(32);
        memcpy(tails_se[i], inputStream, 32);
        inputStream += 32;
    }

    memcpy(*p_tail_sgx, inputStream, 256);
    inputStream += 256;

    memcpy(*p_iv, inputStream, 32);
    inputStream += 32;
}

void serialize_metadata_to_stream(
    unsigned char** meta_stream,
    size_t* p_meta_stream_size,
    int blocks_count,
    int se_blocks_count,
    unsigned char* tail_fk, // 32 bytes
    unsigned char* tail_sk, // 32 bytes
    unsigned char* tails_se[32],
    unsigned char* tail_sgx, //256 bytes
    unsigned char* iv)
{
    int stream_size =
        4       // blocks_count
        + 4     // se_blocks_count
        + 32    // tail_fk
        + 32    // tail_sk
        + 32 * se_blocks_count // tails_se
        + 256   // tail_sgx
        + 32;   // iv
    *meta_stream = (unsigned char*) malloc(stream_size);
    int stream_index = 0;

    unsigned char bc[4];
    pack32(blocks_count, bc);
    memcpy(*meta_stream + stream_index, bc, 4);
    stream_index += 4;

    unsigned char sbc[4];
    pack32(se_blocks_count, sbc);
    memcpy(*meta_stream + stream_index, sbc, 4);
    stream_index += 4;

    memcpy(*meta_stream + stream_index, tail_fk, 32);
    stream_index += 32;

    memcpy(*meta_stream + stream_index, tail_sk, 32);
    stream_index += 32;

    for(int i=0; i<se_blocks_count; i++)
    {
        memcpy(*meta_stream + stream_index, tails_se[i], 32);
        stream_index += 32;
    }

    memcpy(*meta_stream + stream_index, tail_sgx, 256);
    //printf("SSGX : "); print_hex(tail_sgx, 64);
    //printf("stre : "); print_hex(*meta_stream + stream_index, 64);
    stream_index += 256;

    memcpy(*meta_stream + stream_index, iv, 32);
    stream_index += 32;

    *p_meta_stream_size = stream_index;
}


void serialize_metadata_to_redis(
    char* file_name,
    int blocks_count,
    int se_blocks_count,
    unsigned char* tail_fk, // 32 bytes
    unsigned char* tail_sk, // 32 bytes
    unsigned char* tails_se[32],
    unsigned char* tail_sgx, //256 bytes
    unsigned char* iv) // 32 bytes{
{
    unsigned char* m;
    size_t m_size;

    serialize_metadata_to_stream(
        &m,
        &m_size,
        blocks_count,
        se_blocks_count,
        tail_fk, // 32 bytes
        tail_sk, // 32 bytes
        tails_se,
        tail_sgx, //256 bytes
        iv);

    rest_write_metadata(file_name, m, m_size);
}

void deserialize_metadata_file(
    char* file_name,
    int* p_blocks_count,
    int* se_blocks_count,
    unsigned char** tail_fk, // 32 bytes
    unsigned char** tail_sk, // 32 bytes
    unsigned char* tails_se[32], // enc_oeb_index_size bytes
    unsigned char** tail_sgx,
    unsigned char** iv)
{
    unsigned char* m;
    size_t file_size;

    rest_read_metadata(file_name, &m, &file_size);
    //RedisCloud::GetBinary(file_name, &m, &file_size);

    deserialize_metadata_stream(
        m,
        file_size,
        p_blocks_count,
        se_blocks_count,
        tail_fk,
        tail_sk,
        tails_se,
        tail_sgx,
        iv);
}
