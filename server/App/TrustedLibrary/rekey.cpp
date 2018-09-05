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
    usleep(2 * 1000 * 1000);

    // save new metadata
    // Cassandra::insert_meta
}

void read_from_storage(std::string key, unsigned char** value, size_t* p_size)
{
}

void write_to_storage(std::string key, unsigned char* value, size_t size)
{
}

int ocall_get_block(char* key, unsigned char **content, int s)
{
    // TODO : query cassandra for block
    return 0;
}

void ocall_put_block(char* key, unsigned char* content, int content_size)
{
    // TODO : update query for block in cassandra
}
