/*
 * Copyright (C) 2011-2017 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


#include "../App.h"
#include "Enclave_u.h"

#include <vector>
#include <string>

#include <hiredis.h>

#include <fstream>
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem::v1;

static std::string path = "/home/stefan/code/aont/client/tmp_storage/";


static inline void print_hex(unsigned char *h, int l)
{
    for (int i=0; i<l; i++)
        printf("%02X", h[i]);
    printf("\n");
}


#define LOCALHOST  "127.0.0.1"


class RedisCloud
{
    private:
        RedisCloud() {}
        static redisContext *c;

    public:
        static void Init()
        {
            struct timeval timeout = { 1, 500000 }; // 1.5 seconds

            c = redisConnectWithTimeout(LOCALHOST, 6379, timeout);
            if (c == NULL || c->err) {
                if (c) {
                    printf("Connection error: %s\n", c->errstr);
                    redisFree(c);
                } else {
                    printf("Connection error: can't allocate redis context\n");
                }
                exit(1);
            }
        }

        static void Bye()
        {
            redisFree(c);
        }

        static void PutText(std::string key, std::string value)
        {
            redisReply *reply;
            reply = (redisReply*) redisCommand(c,"SET %b %b",
                key.c_str(), (size_t) key.length(),
                value.c_str(), value.length());

            freeReplyObject(reply);
        }

        static void PutBinary(std::string key, unsigned char* data, size_t size)
        {
            std::string s((char*)data, size);
            PutText(key, s);
        }

        static unsigned char* GetBinary(std::string key, unsigned char** p_val, size_t* p_size)
        {
            redisReply *reply;
            reply = (redisReply*) redisCommand(c,"GET %s", key.c_str());
            *p_size = reply->len;
            *p_val = new unsigned char[*p_size];
            memcpy((char*)(*p_val), reply->str, reply->len);
            freeReplyObject(reply);
        }

        static void FlushAll()
        {
            redisCommand(c,"flushall");
            redisCommand(c,"flushdb");
        }
};

redisContext* RedisCloud::c;

void read_from_storage(std::string key, unsigned char** value, size_t* p_size)
{
    RedisCloud::GetBinary(key, value, p_size);
    return;

    std::string full_key_name = path + key;
    std::ifstream s(full_key_name);
    s.seekg(0, std::ifstream::end);
    int file_size = s.tellg();
    s.seekg(0);
    (*value) = (unsigned char*) malloc(file_size);
    s.read(reinterpret_cast<char*>(*value), file_size);
    (*p_size) = file_size;
    printf("FILE SIZE = %d\n", file_size);
    s.close();
}

void write_to_storage(std::string key, unsigned char* value, size_t size)
{
    RedisCloud::PutBinary(key, value, size);
    return;

    std::string full_key_name = path + key;
    std::ofstream s(full_key_name);
    s.write(reinterpret_cast<const char*>(value), size);
    s.close();
}

int ocall_get_block(char* key, unsigned char **content, int s)
{
    clock_t begin = clock();

    unsigned char** c;

    //printf("OCALL GET BLOCK Key %s\n", key);
    size_t content_size = 0;
    std::string k(key);
    read_from_storage(k, content, &content_size);
    //printf("Param : %p\n",c);
    s = content_size;

    //printf("CCCCC : "); print_hex(*content, 32);

    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("OCALL READ %f\n", time_spent);

    return content_size;
}

void ocall_put_block(char* key,
    unsigned char* content, int content_size)
{
    clock_t begin = clock();
//printf("OCALL PUT BLOCK Key %s\n", key);
    std::string k(key);
    write_to_storage(k, content, content_size);

    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("%d OCALL WRITE %f\n", content_size, time_spent);
}

void get_all_metadata_keys(std::vector<std::string>& metadata_keys)
{
    // todo: get keys in redis
    return;

    metadata_keys.clear();
    for (auto & p : fs::directory_iterator(path))
        if (p.path().string().find(".metadata") != std::string::npos)
        {
            std::string s = p.path().string();
            s.replace(0, path.size(), "");
            metadata_keys.push_back(s);
        }
}


void listen_and_rekey(void)
{

    printf("Get all keys ...\n");
/*
    std::vector<std::string> metadata {
        "file_4096.metadata",
        "file_262144.metadata",
        "file_524288.metadata",
        "file_1048576.metadata",
        "file_2097152.metadata",
        "file_4194304.metadata"};
*/
    std::vector<std::string> metadata { "file_1048576.metadata" };

    RedisCloud::Init();

    //std::vector<std::string> metadata;
    //get_all_metadata_keys(metadata);

    for(int i=0; i<metadata.size(); i++)
    {
        printf("Processing [%s]\n", metadata[i].c_str());

        unsigned char* meta_stream;
        size_t meta_size;
        read_from_storage(metadata[i], &meta_stream, &meta_size);

        clock_t begin = clock();

        sgx_status_t ret = SGX_ERROR_UNEXPECTED;
        ret = ecall_worker_re_key(global_eid,
            (char*) metadata[i].c_str(),
            metadata[i].size(),
            (char*) meta_stream,
            meta_size);

        clock_t end = clock();
        double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("TOTAL TIME : %f\n", time_spent);

        if (ret != SGX_SUCCESS)
            abort();
    }

    RedisCloud::Bye();
}
