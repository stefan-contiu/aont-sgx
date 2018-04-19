#include "redis.h"
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
    int* p_enc_oeb_index_size,
    unsigned char** tail_fk, // 32 bytes
    unsigned char** tail_oeb, // 32 bytes
    unsigned char** enc_oeb_index, // enc_oeb_index_size bytes
    unsigned char** iv)
{
    *p_blocks_count = unpack32(inputStream);
    inputStream += 4;
    *p_enc_oeb_index_size = unpack32(inputStream);
    inputStream += 4;

    (*tail_fk) = (unsigned char*) malloc(32);
    (*tail_oeb) = (unsigned char*) malloc(32);
    (*iv) = (unsigned char*) malloc(32);
    (*enc_oeb_index) = (unsigned char*) malloc(*p_enc_oeb_index_size);

    memcpy(*tail_fk, inputStream, 32);
    inputStream += 32;

    memcpy(*tail_oeb, inputStream, 32);
    inputStream += 32;

    memcpy(*enc_oeb_index, inputStream, *p_enc_oeb_index_size);
    inputStream += *p_enc_oeb_index_size;

    memcpy(*iv, inputStream, 32);
    inputStream += 32;
}


void serialize_metadata_to_stream(
    unsigned char** meta_stream,
    int* p_meta_stream_size,
    int blocks_count,
    int enc_oeb_index_size,
    unsigned char* tail_fk, // 32 bytes
    unsigned char* tail_oeb, // 32 bytes
    unsigned char* enc_oeb_index, // enc_oeb_index_size bytes
    unsigned char* iv
)
{

    //print_hex(enc_oeb_index, 256);

    int stream_size = 4 + 4 + 32 + 32 + 32 + enc_oeb_index_size;
    *meta_stream = (unsigned char*) malloc(stream_size);
    int stream_index = 0;

    unsigned char bc[4];
    pack32(blocks_count, bc);
    memcpy(*meta_stream + stream_index, bc, 4);
    stream_index += 4;

    unsigned char bi[4];
    pack32(enc_oeb_index_size, bi);
    memcpy(*meta_stream + stream_index, bi, 4);
    stream_index += 4;

    memcpy(*meta_stream + stream_index, tail_fk, 32);
    stream_index += 32;
    memcpy(*meta_stream + stream_index, tail_oeb, 32);
    stream_index += 32;
    memcpy(*meta_stream + stream_index, enc_oeb_index, enc_oeb_index_size);
    stream_index += enc_oeb_index_size;
    memcpy(*meta_stream + stream_index, iv, 32);
    stream_index += 32;

    *p_meta_stream_size = stream_index;
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

    unsigned char* m;
    int m_size;
    serialize_metadata_to_stream(
        &m,
        &m_size,
        blocks_count,
        enc_oeb_index_size,
        tail_fk, // 32 bytes
        tail_oeb, // 32 bytes
        enc_oeb_index, // enc_oeb_index_size bytes
        iv);

    //printf("META SERIALIZE : "); print_hex(m, m_size);
    //printf("SERIALLIZED META : %d\n", m_size);
    RedisCloud::PutBinary(file_name, m, m_size);

    // old file implementation
    //std::ofstream s(full_key_name);
    //s.write(reinterpret_cast<const char*>(m), m_size);
    //s.close();
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

    /*
    std::string full_key_name(file_name);
    full_key_name = "tmp_storage/" + full_key_name;

    std::ifstream s(full_key_name);
    s.seekg(0, std::ifstream::end);
    int file_size = s.tellg();
    s.seekg(0);
*/


    unsigned char* m;
    size_t file_size;

//    clock_t begin = clock();

    RedisCloud::GetBinary(file_name, &m, &file_size);

//    clock_t end = clock();
//    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;



//    printf("GET BINRARY (%f)\n", time_spent);

    //unsigned char* m; // = (unsigned char*) malloc(file_size);
//    s.read(reinterpret_cast<char*>(m), file_size);


//    printf("DESERIALIZED META : %d\n", file_size);
//    printf("META FROM REDIS : "); print_hex(m, file_size);

    deserialize_metadata_stream(
        m,
        file_size,
        p_blocks_count,
        p_enc_oeb_index_size,
        tail_fk, // 32 bytes
        tail_oeb, // 32 bytes
        enc_oeb_index, // enc_oeb_index_size bytes
        iv);

//    s.close();
}
