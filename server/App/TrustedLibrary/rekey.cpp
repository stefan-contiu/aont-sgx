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

    usleep(2 * 1000 * 1000);

    // go to the enclave

    // decrypt the tail_SGX : 

    // ocall to read block from the storage

    // re-encrypt the block

    // ocall to save block to the storage

    // ocall to save 

}

void read_from_storage(std::string key, unsigned char** value, size_t* p_size)
{
}

void write_to_storage(std::string key, unsigned char* value, size_t size)
{
}

int ocall_get_block(char* key, unsigned char **content, int s)
{
    return 0;
}

int ocall_get_block_ex(char* k, unsigned char **content, int size)
{
    return 0;
}

void ocall_put_block(char* key, unsigned char* content, int content_size)
{
}

int ocall_get_metadata(char* key, unsigned char **content, int s)
{
    return 0;
}

void ocall_put_metadata(char* key, unsigned char* content, int content_size)
{
}

void get_all_metadata_keys(std::vector<std::string>& metadata_keys)
{
}

int worker_loop(int c, char** a)
{
    //
    printf("UWORKER> Started. Hooking to ZooKeeper ...\n");
}

