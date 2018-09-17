#ifndef CASS_H
#define CASS_H

#include <cassandra.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <vector>
#include <string>

#define CASSANDRA_CONTACTS "192.168.1.115,192.168.1.116"

class Cassandra
{
private:
	Cassandra() {}
	static CassCluster* cluster;
	static CassSession* session;
	static CassFuture* connect_future;

public:
	static void Init()
	{
  		cluster = cass_cluster_new();
  		session = cass_session_new();
  		cass_cluster_set_contact_points(cluster, CASSANDRA_CONTACTS);
  		connect_future = cass_session_connect_keyspace(session, cluster, "aont");
		CassError rc = cass_future_error_code(connect_future);
//		printf("Connect result: %s\n", cass_error_desc(rc));
	}

	static void ClearDb()
	{
		CassStatement* statement = cass_statement_new("TRUNCATE meta", 0);
    		CassFuture* query_future = cass_session_execute(session, statement);
    		CassError rc = cass_future_error_code(query_future);

		statement = cass_statement_new("TRUNCATE blocks", 0);
		query_future = cass_session_execute(session, statement);
		rc = cass_future_error_code(query_future);

		cass_statement_free(statement);
    		cass_future_free(query_future);

	}

	static void Bye()
	{
  		cass_future_free(connect_future);
  		cass_session_free(session);
		cass_cluster_free(cluster);
	}

        static std::vector<std::string> get_all_files();

	static void insert_meta(char* file_name, int blocks_count, int se_blocks_count,
    		unsigned char* tail_fk, unsigned char* tail_sk, unsigned char* tails_se[32], unsigned char* tail_sgx);
	static void update_meta(char* file_name, unsigned char* tail_sk);
	static void get_meta(char* file_name, int* blocks_count, int* se_blocks_count,
    		unsigned char** tail_fk, unsigned char** tail_sk, unsigned char* tails_se[32], unsigned char** tail_sgx);

	static void insert_block(char* block_name, unsigned char* data, size_t size);
	static void update_block(char* block_name, unsigned char* data, size_t size);
	static void get_block(char* block_name, unsigned char** data, size_t* data_size);
};

#endif
