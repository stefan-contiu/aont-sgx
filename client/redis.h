#ifndef REDIS_H
#define REDIS_H

//#include <cpp_redis/cpp_redis>
#include <string>
#include <string.h>

#include <hiredis.h>

#define FUJI_SERVER  "192.168.1.100"
#define LOCALHOST  "127.0.0.1"


class RedisCloud
{
    private:
        RedisCloud() {}
        static redisContext *c;
//        static cpp_redis::redis_client client;

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



//            client.connect(FUJI_SERVER, 6379, [](cpp_redis::redis_client&) {
//              std::cout << "client disconnected (disconnection handler)" << std::endl;
//            });
        }

        static void Bye()
        {
            //client.disconnect();
            /* Disconnects and frees the context */
            redisFree(c);

        }

        static void PutText(std::string key, std::string value)
        {
            //printf("=%d\n", value.length());

            redisReply *reply;
            reply = (redisReply*) redisCommand(c,"SET %b %b",
                key.c_str(), (size_t) key.length(),
                value.c_str(), value.length());

            //reply = (redisReply*) redisCommand(c,"SET %s \"%s\"", key.c_str(), value.c_str());
            freeReplyObject(reply);

//            client.set(key, value);
            //client.commit();
            //client.sync_commit();
        }

        static void GetText(std::string key)
        {
            /*
            redisReply *reply;
            reply = (redisReply*) redisCommand(c,"GET %s", key.c_str());
            //printf("GET foo: %s\n", reply->str);
            std::string s(reply->str);
            printf("Returned object has %d\n", s.length());
            freeReplyObject(reply);
*/
            /*
            std::string value;
            client.get(key, [&value](cpp_redis::reply& reply) {
                value = reply.as_string();
            });
            client.sync_commit();
            //client.commit();
            printf("%d response \n", value.length());
            return value;
            */
        }

        static void PutBinary(std::string key, unsigned char* data, size_t size)
        {
            /*
            std::string s((char*)data ,size);
            client.set(key, s);
            client.commit();
            */

            std::string s((char*)data, size);
            PutText(key, s);
        }

        static unsigned char* GetBinary(std::string key, unsigned char** p_val, size_t* p_size)
        {
            //printf("GET BINARY KEY %s\n", key.c_str());
            redisReply *reply;
            reply = (redisReply*) redisCommand(c,"GET %s", key.c_str());
            //printf("GET foo: %s\n", reply->str);

            //printf("Returned object has %d\n", s.length());

            *p_size = reply->len;
            *p_val = new unsigned char[*p_size];
            memcpy((char*)(*p_val), reply->str, reply->len);

            freeReplyObject(reply);
        }

        static void FlushAll()
        {
            redisCommand(c,"flushall");
            redisCommand(c,"flushdb");
            //client.send({ "flushall" });
        }

        static void Commit()
        {
            //client.sync_commit();
        }
};

#endif
