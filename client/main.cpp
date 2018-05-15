#include "redis.h"
#include <stdio.h>
#include <stdlib.h>
#include "crypto.h"

#include "client.h"

#include <fstream>
#include <cstring>
#include <vector>

#include <iostream>
#include <sstream>

#include "serialization.h"
#include "storage.h"

#include <algorithm>

#include <hiredis.h>


std::vector<int> block_sizes{1024, 4 * 1024, 256 * 1024, 512 * 1024, 1024*1024, 2 * 1024*1024, 4 * 1024 * 1024};

void benchmark_write(int reps=10)
{
    unsigned char* old_gk = gen_random_bytestream(32); //(unsigned char*) "12345678901234567890123456789012";
    //std::vector<int> block_sizes{1024, 4 * 1024, 256 * 1024, 512 * 1024, 768*1024, 1024 * 1024, 2 * 1024 * 1024, 4*1024*1024};

    //std::vector<int> block_sizes{1024 * 1024};

    std::vector<int> read;

    //write_file("file.txt", old_gk, rsaPublicKey, strlen(rsaPublicKey), block_sizes[4]);

    // read input file
    std::ifstream t("file16.txt"); //file64.txt
    std::stringstream buffer;
    buffer << t.rdbuf();
    std::string s = buffer.str();

    RedisCloud::FlushAll();
    for(int i=0; i<block_sizes.size(); i++)
    {
        std::vector<double> write_total;
        std::vector<double> write_m0;
        std::vector<double> write_m1;

        std::vector<double> read_total;
        std::vector<double> read_storage;
        std::vector<double> read_aes;

        for(int r=0; r<reps; r++)
        {
            std::string file_name = "file_" + std::to_string(i) + "_" + std::to_string(r);
            //std::string file_name = "file14.txt";

            clock_t begin = clock();

            std::pair<double, double> t = write_file_aes((char*)file_name.c_str(), s,
                old_gk, rsaPublicKey, strlen(rsaPublicKey), block_sizes[i]);

            clock_t end = clock();
            double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;

            clock_t rbegin = clock();
            std::pair<double, double> tr = read_file_aes((char*)file_name.c_str(),
                old_gk, "temp.dat", block_sizes[i]);
            clock_t rend = clock();
            double rtime_spent = (double)(rend - rbegin) / CLOCKS_PER_SEC;
        //    printf("%f\n", rtime_spent);

            write_total.push_back(time_spent);
            write_m0.push_back(t.first);
            write_m1.push_back(t.second);

            read_total.push_back(rtime_spent);
            read_storage.push_back(tr.first);
            read_aes.push_back(tr.second);
            RedisCloud::FlushAll();
        }

        std::sort(write_total.begin(), write_total.end(), std::greater<int>());
        std::sort(write_m0.begin(), write_m0.end(), std::greater<int>());
        std::sort(write_m1.begin(), write_m1.end(), std::greater<int>());

        std::sort(read_total.begin(), read_total.end(), std::greater<int>());
        std::sort(read_storage.begin(), read_storage.end(), std::greater<int>());
        std::sort(read_aes.begin(), read_aes.end(), std::greater<int>());

        printf("%f,%f,%f,", write_total[reps/2] - write_m0[reps/2] - write_m1[reps/2], write_m0[reps/2], write_m1[reps/2]);
        printf("%f,%f,%f\n", read_total[reps/2] - read_storage[reps/2] - read_aes[reps/2], read_storage[reps/2], read_aes[reps/2]);

        RedisCloud::FlushAll();

/*
        begin = clock();
        read_file("file.txt", old_gk, "temp.dat", block_sizes[i]);
        end = clock();
        time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
        read.push_back(time_spent);
        printf("%f\n", time_spent);
*/
    }
}

void feed_rekey(int block_size)
{
    // read input file
    std::ifstream t("file16.txt"); //file64.txt
    std::stringstream buffer;
    buffer << t.rdbuf();
    std::string s = buffer.str();

    std::string file_name = "file_" + std::to_string(block_size);
    //std::string file_name = "file14.txt";

    unsigned char* old_gk = gen_random_bytestream(32);

    write_file((char*)file_name.c_str(), s,
        old_gk, rsaPublicKey, strlen(rsaPublicKey), block_size, 1);

    t.close();
}

int functional_tests(int block_size_in_bytes, int se_count)
{
    printf("AONT Client Functional Tests \n");
    srand (time(NULL));

    std::ifstream t("file.dat"); //file64.txt
    std::stringstream buffer;
    buffer << t.rdbuf();
    std::string s = buffer.str();

    std::string file_name = "file_" + std::to_string(block_size_in_bytes);

    unsigned char* old_gk = (unsigned char*) "12345678901234567890123456789012";

    write_file((char*)file_name.c_str(), s,
        old_gk, rsaPublicKey, strlen(rsaPublicKey), block_size_in_bytes, se_count);

    t.close();


    //write_file("file.dat", old_gk, rsaPublicKey, strlen(rsaPublicKey), 256 * 1024);
    //read_file("file.dat", old_gk, "temp.dat", 256 * 1024);

    //re_key(old_gk, new_gk);
    //read_file("file.dat", new_gk, "temp_rekeyed.dat");
}

int storage_test()
{
    size_t size = 1024 * 1024 * 16;
    unsigned char* m = (unsigned char*) malloc(size);
    m = gen_random_bytestream(size);


    clock_t begin = clock();
    write_to_storage("test", m, size);
    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("WRITE : %f\n", time_spent);

    unsigned char* mm;
    size_t ss;
    begin = clock();
    read_from_storage("test", &mm, &ss);
    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("READ  : %f\n", time_spent);
}


int main()
{
    RedisCloud::Init();

    RedisCloud::FlushAll();

    //for(int i=1; i<block_sizes.size(); i++)
    //    feed_rekey(block_sizes[i]);

    //benchmark_aes();
    //benchmark_write();
    functional_tests(64 * 1024, 3);
    //benchmark_write();
    //storage_test();

    RedisCloud::Bye();
}
