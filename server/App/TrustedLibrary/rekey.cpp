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


static inline void print_hex(unsigned char *h, int l)
{
    for (int i=0; i<l; i++)
        printf("%02X", h[i]);
    printf("\n");
}

void re_key(char* file_name)
{
    printf("Re-encryption of %s\n", file_name);

    // get metadata for the file from cassandra
    int blocks_count;
    int se_blocks_count;
    unsigned char* tail_fk;
    unsigned char* tail_sk;
    unsigned char* tails_se[32];
    unsigned char* tail_sgx;
    Cassandra::get_meta(file_name, &blocks_count, &se_blocks_count, &tail_fk, &tail_sk, (unsigned char**)tails_se, &tail_sgx);
    printf("File %s blocks count : %d\n", file_name, blocks_count);
    printf("Super Encrypted Blocks count : %d\n", se_blocks_count);
//    printf("Tail SGX : "); print_hex(tail_sgx, 256);


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

    // mock work
    //usleep(2 * 1000 * 1000);

    // save new metadata
    // Cassandra::insert_meta
}

int ocall_get_block(char* key, unsigned char **content, int p_size)
{
    printf("Fetching block from cassandra : %s\n", key);
    size_t n;
    Cassandra::get_block(key, content, &n);
    p_size = (int) n;	
    return p_size;
}

void ocall_put_block(char* key, unsigned char* content, int content_size)
{
    printf("Writing block to cassandra : %s\n", key);
    Cassandra::update_block(key, content, (size_t) content_size);
}
