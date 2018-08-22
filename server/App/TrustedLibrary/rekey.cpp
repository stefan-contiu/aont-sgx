#include "../App.h"
#include "Enclave_u.h"

#include <vector>
#include <string>

#include <tgmath.h>

#include <signal.h>
#include <hiredis.h>
#include <async.h>
#include <adapters/libevent.h>

#include <unistd.h>

#include <set>
#include <map>

#include <fstream>
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem::v1;

// ---------------------------------------------------------
#define REDIS_CLOUD "192.168.1.112"
static std::string path = "/hdd/";
// ---------------------------------------------------------

static inline void print_hex(unsigned char *h, int l)
{
    for (int i=0; i<l; i++)
        printf("%02X", h[i]);
    printf("\n");
}


void worker_execute_job(char* workerName, char* params, size_t size);

int WORKERS_ALIVE = 0;

std::set<std::string> terminated;
int FINAL_TERMINATION = 0;

class RedisCloud
{
    private:
        RedisCloud() {}
        static redisContext *c;
        static redisAsyncContext *async_c;
        static struct event_base* base;

    public:
        static void Init()
        {
            struct timeval timeout = { 1, 500000 }; // 1.5 seconds

            c = redisConnectWithTimeout(REDIS_CLOUD, 6379, timeout);
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

        static void pubCallback(redisAsyncContext *c, void *r, void *privdata) {
          redisReply *reply = (redisReply*)r;
          if (reply == NULL){
              printf("Response not received.\n");
            //cout<<"Response not recev"<<endl;
            return;
          }
          //printf("Message published.\n");
          //cout<<"message published"<<endl;
          redisAsyncDisconnect(async_c);
        }

        static void Publish(std::string key, std::string& value)
        {
            signal(SIGPIPE, SIG_IGN);
            struct event_base* base = event_base_new();
            async_c = redisAsyncConnect(REDIS_CLOUD, 6379);
            if (async_c->err) {
                printf("error: %s\n", async_c->errstr);
                return;
            }
            redisLibeventAttach(async_c, base);

            std::string cmd = "PUBLISH ";
            cmd.append(key);
            cmd.append(" ");
            cmd.append(value);

            //printf("value : %s\n", value.c_str());
            //printf("to redis : %s\n", cmd.c_str());
            redisAsyncCommand(async_c,
                       pubCallback,
                       (char*)"pub", cmd.c_str());

            event_base_dispatch(base);
        }


        static void _subCallback(redisAsyncContext *c, void *reply, void *privdata)
        {
            redisReply *r = (redisReply*) reply;
            if (reply == NULL) return;

            if (r->type == REDIS_REPLY_ARRAY) {

                if (r->element[2]->str != NULL)
                {

                    if (strncmp(r->element[1]->str, "termination", 11) == 0)
                    {
                        printf("MASTER FINDS OUT TERMINATION %s : %s !!!\n", r->element[1]->str, r->element[2]->str);

                        // master handles termination ACK from workers
                        if (strlen(r->element[2]->str) > 0)
                        {
                            terminated.insert(r->element[2]->str);

                            if (terminated.size() == WORKERS_ALIVE)
                            {
                                redisAsyncDisconnect(async_c);
                            }
                        }
                    }
                    else
                    {
                        // workers handle job request from master
                        // FIX : we assume that the job string is null terminated - this should change if we
                        // broadcast some binary data
                        size_t job_size = strlen(r->element[2]->str);
                        char* job = (char*) malloc(job_size);
                        memcpy(job, r->element[2]->str, job_size);
                        worker_execute_job(r->element[1]->str, job, job_size);
                    }
                }
            }

            //redisAsyncDisconnect(async_c);
        }

        static void Subscribe(std::string key)
        {
            signal(SIGPIPE, SIG_IGN);
            struct event_base* base = event_base_new();

            async_c = redisAsyncConnect(REDIS_CLOUD, 6379);
            if (async_c->err) {
                printf("error: %s\n", async_c->errstr);
                return;
            }
            redisLibeventAttach(async_c, base);

            std::string cmd = "SUBSCRIBE ";
            cmd.append(key);

            redisAsyncCommand(async_c, _subCallback, (char*)"sub", cmd.c_str());
            event_base_dispatch(base);
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

        static void FillMetadata(std::vector<std::string>& metadata)
        {
            metadata.clear();
            redisReply *reply;
            reply = (redisReply*) redisCommand(c,"KEYS *.metadata");
            for(int i=0;i< reply->elements;i++)
            {
                //std::string* s = new std::string(reply->element[i]->str);
                //printf("REDIS KEY : %s\n", s->c_str());
                metadata.push_back(reply->element[i]->str);
            }
        }
};

redisContext* RedisCloud::c;
redisAsyncContext *RedisCloud::async_c;
struct event_base* RedisCloud::base;

void read_from_storage(std::string key, unsigned char** value, size_t* p_size)
{
    std::string full_key_name = path + key;
    std::ifstream s(full_key_name);
    s.seekg(0, std::ifstream::end);
    int file_size = s.tellg();
    //printf("FILE %s SIZE = %d\n", full_key_name.c_str(), file_size);

    s.seekg(0);

    // TODO : check caller and make sure we don't double allocate
    (*value) = (unsigned char*) malloc(file_size);
    s.read(reinterpret_cast<char*>(*value), file_size);
    (*p_size) = file_size;
    s.close();
}

void write_to_storage(std::string key, unsigned char* value, size_t size)
{
    std::string full_key_name = path + key;
    std::ofstream s(full_key_name);
    s.write(reinterpret_cast<const char*>(value), size);
    s.close();
}

int ocall_get_block(char* key, unsigned char **content, int s)
{
    clock_t begin = clock();

    //printf("OCALL GET BLOCK Key %s\n", key);
    size_t content_size = 0;
    std::string k(key);
    read_from_storage(k, content, &content_size);
    //printf("Param : %p\n",c);
    s = content_size;

    //printf("CCCCC : "); print_hex(*content, 32);

    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    //printf("OCALL READ %f\n", time_spent);

    return content_size;
}

int ocall_get_block_ex(char* k,
    unsigned char **content, int size)
{
    //print_hex(*content, 64);
    //memcpy(content, "123456", 6);
    //
    //return 0;
    //size_t content_size = 0;
    //memcpy(content, "123456", 6);

    std::string key(k);
    std::string full_key_name = path + key;
    std::ifstream s(full_key_name);
    s.seekg(0, std::ifstream::end);
    int file_size = s.tellg();
    size = file_size;
    //printf("FILE %s SIZE = %d\n", full_key_name.c_str(), file_size);

    s.seekg(0);

    s.read(reinterpret_cast<char*>(*content), file_size);
    //(*p_size) = file_size;
    s.close();

    return file_size;
}

void ocall_put_block(char* key,
    unsigned char* content, int content_size)
{
    // TODO : I think that content should be passed as an "in" in EDL
    clock_t begin = clock();
//printf("OCALL PUT BLOCK Key %s\n", key);
    std::string k(key);
    write_to_storage(k, content, content_size);

    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
//    printf("%d OCALL WRITE %f\n", content_size, time_spent);
}

int ocall_get_metadata(char* key, unsigned char **content, int s)
{
    unsigned char** c;
    size_t content_size = 0;
    std::string k(key);
    RedisCloud::GetBinary(key, content, &content_size);
    s = content_size;
    return content_size;
}

void ocall_put_metadata(char* key, unsigned char* content, int content_size)
{
    std::string k(key);
    RedisCloud::PutBinary(k, content, content_size);
}

void get_all_metadata_keys(std::vector<std::string>& metadata_keys)
{
    metadata_keys.clear();
    for (auto & p : fs::directory_iterator(path))
        if (p.path().string().find(".metadata") != std::string::npos)
        {
            std::string s = p.path().string();
            s.replace(0, path.size(), "");
            metadata_keys.push_back(s);
        }
}


void onMessage(redisAsyncContext *c, void *reply, void *privdata) {
    redisReply *r = (redisReply*) reply;
    if (reply == NULL) return;

    if (r->type == REDIS_REPLY_ARRAY) {
        for (int j = 0; j < r->elements; j++) {
            printf("%u) %s\n", j, r->element[j]->str);
        }
    }
}

void subCallback(redisAsyncContext *c, void *r, void *privdata) {

  redisReply *reply = (redisReply*)r;
  if (reply == NULL){
    printf("Response not recev\n");
    return;
  }
  if(reply->type == REDIS_REPLY_ARRAY & reply->elements == 3)
  {
    if(strcmp( reply->element[0]->str,"subscribe") != 0)
    {
      printf("Message received -> \n");//<<
        //reply->element[2]->str<<"( on channel : "<<reply->element[1]->str<<")"<<endl;
    }
  }
}

/*
[ ] Store Metadata Recipes in REDIS, blocks on HDD
[ ] Master : first publishes an ALIVE_QUERY, subscribers (workers) return with their SGX_PUB_KEY.
Master uses the list of respondents to perform a GroupSeal operation (hybrid encryption based).

[ ] Do just single encryption, not super encryption: randomly chose K shielded blocks encrypted by GK, all the rest encrypted by FK.
*/

void master_loop(std::vector<std::string> workers, int do_full_aes)
{
    printf("MASTER> Starting master loop\n");

    struct timeval diff, startTV, endTV;
    gettimeofday(&startTV, NULL);

    RedisCloud::Init();

    // master is always ON
    // when workers are joining they publish a h(PK)
    // master is always keeping a list of workers

    // how many enclaves are up ? SGX_WORKER_0, SGX_WORKER_1, SGX_WORKER_2, ...
    WORKERS_ALIVE = workers.size(); //
    printf("MASTER> workers available : %d\n", WORKERS_ALIVE);

    // ecall : generate a new group key. GroupSeal: old_gk, new_gk.
    unsigned char group_sealed_keys[64];

    // get all the metadata files
    std::vector<std::string> metadata_files;
    RedisCloud::FillMetadata(metadata_files);
    int M = metadata_files.size();
    printf("MASTER> total files to re-key : %d\n", M);

    // for each file bring its storage ID
    std::map<std::string, std::vector<std::string> > files_per_storage;
    for(int i=0; i<metadata_files.size(); i++)
    {
        // cut .metadata suffix
        int l = metadata_files[i].length();
        std::string s = metadata_files[i].substr(0, l - 9);

        // get storage from redis
        unsigned char* st;
        size_t size;
        RedisCloud::GetBinary(s, &st, &size);
        std::string storage((char*)st, size);

        // if storage exists as a key, push_back to value
        files_per_storage[storage].push_back(metadata_files[i]);
        /*
        if (files_per_storage.count(storage) > 0)
        {

        }
        else
        {

        }
        // else create a new value
        */
    }

    // broadcast to each worker its batch
    std::map<std::string, std::vector<std::string>>::iterator it = files_per_storage.begin();
    while (it != files_per_storage.end())
    {
        printf("MASTER> Broadcast to Worker %s : \n", it->first.c_str());

        std::string batch_files = "";
        if (do_full_aes)
        {
            batch_files.append("full_aes:");
        }

        for (int i = 0; i < it->second.size(); i++)
        {
            batch_files += it->second[i] + ";";
        }

        printf("MASTER> Broadcast Value : %s\n", batch_files.c_str());

        // TODO : uncomment this line
        RedisCloud::Publish(it->first, batch_files);

        // go to next worker
        it++;
    }

    // for each

    // for each storage:
    //      * get list of files at storage
    //      * get available Workers at storage
    //      * split list of files per number of workers
    //      * broadcast to each worker its batch
/*
    // split M files into N batches
    int batch_size = (int) ceil((double)M / WORKERS_ALIVE);
    for(int i = 0; i < WORKERS_ALIVE; i++)
    {
        printf("MASTER> Broadcast to Worker %d : \n", i);
        int batch_start = i * batch_size;
        int batch_end   = (i + 1) * batch_size;
        if (batch_end > M)
        {
            batch_end = M;
        }

        // start the message with the group selaed keys
        std::string batch_files = "";
        for (int j = batch_start; j < batch_end; j++)
        {
            batch_files += metadata_files[j] + ";";
            //printf("%s\n", metadata_files[j].c_str());
        }

        RedisCloud::Publish(workers[i], batch_files);
    }
*/
    // master subscribes and listens to termination of all workers
    RedisCloud::Subscribe("termination");

    RedisCloud::Bye();
    printf("MASTER> All Workers Terminated!\n");
    gettimeofday(&endTV, NULL);
    timersub(&endTV, &startTV, &diff);
    printf("*** time taken = %ld.%ld (s)\n", diff.tv_sec, diff.tv_usec);



/*
    // TODO check for termination ?
    return;

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
*/
}

void worker_execute_job(char* worker_name, char* params, size_t size)
{
    printf("WORKER %s> Do work...\n", worker_name);
    //printf("WORKER %s> Pushing to sgx :%s\n", worker_name, params);
    //printf("WORKER %s> Size to sgx :%d\n", worker_name, size);

    // go to SGX enclave and do batch processing

    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    ret = ecall_worker_re_key(global_eid, params, size);
    if (ret != SGX_SUCCESS) abort();

    // hack during dev time : sleep to simulate SGX work
    //usleep(1000);

    // signal to master that work is done
    std::string worker_done_key = "termination";
    std::string worker_done_value = worker_name;
    RedisCloud::Publish(worker_done_key, worker_done_value);
}


int zk_worker_loop(int c, char** a)
{
    //
    printf("WORKER> Started...\n");

    // subscribe to worker_name
    //RedisCloud::Init();
    //RedisCloud::Subscribe(worker_name);
}

