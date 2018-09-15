//#include "redis.h"
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

#include <random>

#include "cass.h"

#include <chrono> 
using namespace std::chrono; 


std::vector<int> block_sizes{1024, 4 * 1024, 256 * 1024, 512 * 1024, 1024*1024, 2 * 1024*1024, 4 * 1024 * 1024};

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
    t.close();
    printf("Loaded test file of size %d \n", (int)s.size());

    std::string file_name = "file_" + std::to_string(block_size_in_bytes);

    unsigned char* old_gk = (unsigned char*) "12345678901234567890123456789012";

    printf("Write file ...\n");
    write_file((char*)file_name.c_str(), s,
        old_gk, rsaPublicKey, strlen(rsaPublicKey), block_size_in_bytes, se_count);
    printf("File written!\n");

    std::string response;
    read_file(file_name, response, old_gk, block_size_in_bytes);

    // check that same file size was returned
    if (s.size() != response.size())
    {
        printf("TEST FAILED : write with %d bytes, read returned %d bytes.",
            (int)s.size(), (int)response.size());
        return -1;
    }

    // check that the same content was returned
    unsigned char sha_write[32];
    sgx_sha256((unsigned char*) s.c_str(), s.size(), sha_write);
    unsigned char sha_read[32];
    sgx_sha256((unsigned char*) response.c_str(), response.size(), sha_read);
    if (memcmp((unsigned char*) s.c_str(), (unsigned char*) response.c_str(), 32) != 0)
    {
        printf("TEST FAILED : read returned a different content that written.");
        print_hex(sha_write, 32);
        print_hex(sha_read, 32);
        return -1;
    }

    printf("TEST PASSED\n");
    return 0;
}

int write_files_according_to_distribution(int files_count, std::vector<int> distribution,
    int min_size, int max_size,
    int block_size_in_bytes,
    int se_count,
    std::string file_prefix)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::discrete_distribution<> d(distribution.begin(), distribution.end());
    unsigned char* gk = (unsigned char*) "12345678901234567890123456789012";

    for(int i=0; i<files_count; i++)
    {
        int v = d(gen);
        v = min_size + (v * (max_size - min_size) / distribution.size());
        printf("Writing file %d of %d with size : %d\n", i, files_count, v);

        std::string file_name = file_prefix + "f_" + std::to_string(i) + ".dat";
        unsigned char* random_content = gen_random_bytestream(v * 1024 * 1024);
        std::string file_content((char*)random_content, v * 1024 * 1024);

        write_file((char*)file_name.c_str(), file_content,
            gk, rsaPublicKey, strlen(rsaPublicKey), block_size_in_bytes, se_count);

        free(random_content);
    }
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
    Cassandra::Init();
    Cassandra::ClearDb();
    if (functional_tests(64 * 1024, 3) != 0)
    {
        printf("Execution stopped. Functional tests failed.\n");
        return -1;
    }
    Cassandra::Bye();

    unsigned char* mm;
    size_t ss;
    begin = clock();
    read_from_storage("test", &mm, &ss);
    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("READ  : %f\n", time_spent);
}

int cassandra_functional_test()
{
    Cassandra::Init();
    Cassandra::ClearDb();
    if (functional_tests(64 * 1024, 3) != 0)
    {
        printf("Execution stopped. Functional tests failed.\n");
        return -1;
    }
    Cassandra::Bye();
}

int write_many_files(int files_count, int file_size, int block_size_in_bytes, int se_count)
{
    	Cassandra::Init();
    	Cassandra::ClearDb();

	for(int i=0; i<files_count; i++)
	{
		std::string file_name = "file" + std::to_string(i) + ".dat";
    		unsigned char* gk = (unsigned char*) "12345678901234567890123456789012";
		unsigned char* buffer = (unsigned char*) malloc(file_size);
		std::string s((char*)buffer, file_size);

		// WRITE
		auto t1 = std::chrono::high_resolution_clock::now();
		std::pair<double, double> t;
                t = write_file((char*)file_name.c_str(), s, gk, rsaPublicKey, strlen(rsaPublicKey), block_size_in_bytes, se_count);
		auto t2 = std::chrono::high_resolution_clock::now();
		auto tm = duration_cast<milliseconds>(t2 - t1).count();
		printf("w,%d,%d,%ld,%ld\n", block_size_in_bytes/1024, se_count, tm, (long)t.first);

		// READ
		t1 = std::chrono::high_resolution_clock::now();
    		std::string response;
    		//t = read_file(file_name, response, gk, block_size_in_bytes);
		t2 = std::chrono::high_resolution_clock::now();
		tm = duration_cast<milliseconds>(t2 - t1).count();
		printf("r,%d,%d,%ld,%ld\n", block_size_in_bytes/1024, se_count, tm, 0);// WARNING : (long)t.first);

		free(buffer);
	}

	Cassandra::Bye();
}

int main(int argc, char **argv)
{
	if (argc > 1)
	{
		if (strncmp(argv[1], "-micro", strlen(argv[1])) == 0)
		{
			int file_size_kb = std::stoi(argv[2]);
			int block_size_kb = std::stoi(argv[3]);
			int se_count = std::stoi(argv[4]);
			write_many_files(5, file_size_kb * 1024, block_size_kb * 1024, se_count);
		}
                else {
			printf("usage: -micro file_size_kb block_size_kb se_blocks_count\n");
		}
	}
	else
	{
		cassandra_functional_test();
	}
}

