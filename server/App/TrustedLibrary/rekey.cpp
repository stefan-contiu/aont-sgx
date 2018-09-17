#include "../App.h"
#include "../cass.h"
#include "Enclave_u.h"

#include <vector>
#include <string>

#include <tgmath.h>

#include <unistd.h>

#include <set>
#include <map>

#include <fstream>

#include <chrono>
using namespace std::chrono;

static inline void print_hex(unsigned char *h, int l)
{
    for (int i=0; i<l; i++)
        printf("%02X", h[i]);
    printf("\n");
}

long re_key(char* file_name)
{
	time_outside_sgx = 0;
    auto t1 = std::chrono::high_resolution_clock::now();


//    printf("Re-encryption of %s\n", file_name);

    // get metadata for the file from cassandra
    int blocks_count;
    int se_blocks_count;
    unsigned char* tail_fk;
    unsigned char* tail_sk;
    unsigned char* tails_se[32];
    unsigned char* tail_sgx;
    Cassandra::get_meta(file_name, &blocks_count, &se_blocks_count, &tail_fk, &tail_sk, (unsigned char**)tails_se, &tail_sgx);
//    printf("File %s blocks count : %d\n", file_name, blocks_count);
//    printf("Super Encrypted Blocks count : %d\n", se_blocks_count);
//    printf("Tail SGX : "); print_hex(tail_sgx, 256);
    auto t2 = std::chrono::high_resolution_clock::now();
    time_outside_sgx += duration_cast<milliseconds>(t2 - t1).count();
//    printf("ocall get meta : %ld\n", time_outside_sgx);

    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    ret = ecall_re_key(global_eid,
                file_name,
                blocks_count,
                se_blocks_count,
                tail_fk,
                tail_sk,
                tails_se,
                tail_sgx);
    if (ret != SGX_SUCCESS) abort();

    // save new metadata
    t1 = std::chrono::high_resolution_clock::now();
    Cassandra::insert_meta(file_name, blocks_count, se_blocks_count, tail_fk, tail_sk, 
	tails_se, tail_sgx);
    t2 = std::chrono::high_resolution_clock::now();
    time_outside_sgx += duration_cast<milliseconds>(t2 - t1).count();
    //printf("ocall insert meta : %ld\n", time_outside_sgx);
    return time_outside_sgx;
}

int ocall_get_block(char* key, unsigned char **content, int p_size)
{
    auto t1 = std::chrono::high_resolution_clock::now();

    size_t n;
    Cassandra::get_block(key, content, &n);
    p_size = (int) n;	

    auto t2 = std::chrono::high_resolution_clock::now();
    time_outside_sgx += duration_cast<milliseconds>(t2 - t1).count();
  //  printf("ocall get : %ld\n", time_outside_sgx);

    return p_size;
}

void ocall_put_block(char* key, unsigned char* content, int content_size)
{
    auto t1 = std::chrono::high_resolution_clock::now();

    Cassandra::update_block(key, content, (size_t) content_size);

    auto t2 = std::chrono::high_resolution_clock::now();
    time_outside_sgx += duration_cast<milliseconds>(t2 - t1).count();
//    printf("ocall put : %ld\n", time_outside_sgx);
}
