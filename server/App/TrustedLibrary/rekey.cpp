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

        static void pubCallback(redisAsyncContext *c, void *r, void *privdata) {
          redisReply *reply = (redisReply*)r;
          if (reply == NULL){
              printf("Response not received.\n");
            //cout<<"Response not recev"<<endl;
            return;
          }
          printf("Message published.\n");
          //cout<<"message published"<<endl;
          redisAsyncDisconnect(async_c);
        }

        static void Publish(std::string key, std::string& value)
        {
            signal(SIGPIPE, SIG_IGN);
            struct event_base* base = event_base_new();
            async_c = redisAsyncConnect(LOCALHOST, 6379);
            if (async_c->err) {
                printf("error: %s\n", async_c->errstr);
                return;
            }
            redisLibeventAttach(async_c, base);

            std::string cmd = "PUBLISH ";
            cmd.append(key);
            cmd.append(" ");
            cmd.append(value);

            printf("value : %s\n", value.c_str());
            printf("to redis : %s\n", cmd.c_str());
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
                        // master handles termination ACK from workers
                        if (strlen(r->element[2]->str) > 0)
                        {
                            //printf("SUB MESSAGE %s : %s !!!\n", r->element[1]->str, r->element[2]->str);
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
                        // TODO : fix size the last param
                        worker_execute_job(r->element[1]->str, r->element[2]->str, 0);
                    }
                }
            }

            //redisAsyncDisconnect(async_c);
        }

        static void Subscribe(std::string key)
        {
            signal(SIGPIPE, SIG_IGN);
            struct event_base* base = event_base_new();

            async_c = redisAsyncConnect(LOCALHOST, 6379);
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


void query_alive_workers(std::vector<std::string>& workers_pub_keys)
{
    // publish a WORKER_ALIVE message?
    workers_pub_keys.push_back("w0");
    workers_pub_keys.push_back("w1");
    workers_pub_keys.push_back("w2");
}

void master_loop(void)
{
    struct timeval diff, startTV, endTV;
    gettimeofday(&startTV, NULL);

    RedisCloud::Init();

    // master is always ON
    // when workers are joining they publish a h(PK)
    // master is always keeping a list of workers

    // workers they listen to tasks over a hash of their public key
    std::vector<std::string> workers;
    query_alive_workers(workers);

    // how many enclaves are up ? SGX_WORKER_0, SGX_WORKER_1, SGX_WORKER_2, ...
    WORKERS_ALIVE = workers.size(); //
    printf("MASTER> workers available : %d\n", WORKERS_ALIVE);

    // ecall : generate a new group key. GroupSeal: old_gk, new_gk.
    unsigned char group_sealed_keys[64];

    // get all the metadata files
    std::vector<std::string> metadata_files;
    //RedisCloud::InitAsync();
    RedisCloud::FillMetadata(metadata_files);
    int M = metadata_files.size();
    printf("MASTER> total files to re-key : %d\n", M);

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
        printf("Pub done\n");
    }

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

    // de-serialize parameters - mock the two encryption keys
    unsigned char group_sealed_keys[64];

    // go to SGX enclave and do batch processing
    usleep(5000 * 1000);

    // signal to master that work is done
    std::string worker_done_key = "termination";
    std::string worker_done_value = worker_name;
    RedisCloud::Publish(worker_done_key, worker_done_value);
}

void worker_loop(char* worker_name)
{
    //
    printf("WORKER %s> Started...\n", worker_name);

    // subscribe to worker_name
    RedisCloud::Init();
    RedisCloud::Subscribe(worker_name);
}
