#ifndef SERIALIZATION_H
#define SERIALIZATION_H

//#include <libc_mock/libcpp_mock.h>

#include <cstring>
#include <stdio.h>

static inline void print_hex(unsigned char *h, int l)
{
    for (int i=0; i<l; i++)
        printf("%02X", h[i]);
    printf("\n");
}

extern void long_to_byte_array(unsigned long n, unsigned char* bytes, size_t bytes_size);

extern void byte_array_to_long(unsigned char* bytes, unsigned long* n);

// Serialize/Deserialize to/from Storage Methods ----

void serialize_metadata_to_redis(
    char* file_name,
    int blocks_count,
    int se_blocks_count,
    unsigned char* tail_fk, // 32 bytes
    unsigned char* tail_sk, // 32 bytes
    unsigned char* tails_se[32],
    unsigned char* tail_sgx, //256 bytes
    unsigned char* iv); // 32 bytes

extern void deserialize_metadata_file(
    char* file_name,
    int* p_blocks_count,
    int* se_blocks_count,
    unsigned char** tail_fk, // 32 bytes
    unsigned char** tail_sk, // 32 bytes
    unsigned char* tails_se[32], // enc_oeb_index_size bytes
    unsigned char** tail_sgx,
    unsigned char** iv);

// Binary Stream Methods ----

extern void deserialize_metadata_stream(
    char* inputStream,
    size_t inputStreamSize,
    int* p_blocks_count,
    int* se_blocks_count,
    unsigned char** tail_fk, // 32 bytes
    unsigned char** tail_sk, // 32 bytes
    unsigned char* tails_se[32], // enc_oeb_index_size bytes
    unsigned char** tail_sgx,
    unsigned char** iv);

extern void serialize_metadata_to_stream(
    unsigned char** meta_stream,
    size_t* p_meta_stream_size,
    int blocks_count,
    int se_blocks_count,
    unsigned char* tail_fk, // 32 bytes
    unsigned char* tail_sk, // 32 bytes
    unsigned char* tails_se[32],
    unsigned char* tail_sgx, //256 bytes
    unsigned char* iv);

#endif
