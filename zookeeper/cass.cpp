#include "cass.h"

CassCluster* Cassandra::cluster;
CassSession* Cassandra::session;
CassFuture* Cassandra::connect_future;

void Cassandra::insert_meta(char* file_name, int blocks_count, int se_blocks_count,
	unsigned char* tail_fk, unsigned char* tail_sk, unsigned char* tails_se[32], unsigned char* tail_sgx)
{
    //printf("Writing meta...\n");
    CassStatement* statement
        = cass_statement_new("INSERT INTO meta (file_name, blocks_count, se_blocks_count, tail_fk, tail_sk, tail_se, tail_sgx) VALUES (?, ?, ?, ?, ?, ?, ?)", 7);

    cass_statement_bind_string(statement, 0, file_name);
    cass_statement_bind_int32(statement, 1, blocks_count);
    cass_statement_bind_int32(statement, 2, se_blocks_count);
    cass_statement_bind_bytes(statement, 3, (const cass_byte_t*) tail_fk, 32);
    cass_statement_bind_bytes(statement, 4, (const cass_byte_t*) tail_sk, 32);
    unsigned char* all_se = (unsigned char*) malloc(blocks_count * 32);
    for(int i=0; i<se_blocks_count; i++)
    {
        memcpy(all_se + (32 * i), tails_se[i], 32);
    }
    cass_statement_bind_bytes(statement, 5, (const cass_byte_t*) all_se, blocks_count * 32);
    cass_statement_bind_bytes(statement, 6, (const cass_byte_t*) tail_sgx, 32);

    CassFuture* query_future = cass_session_execute(session, statement);
    CassError rc = cass_future_error_code(query_future);
    //printf("Query result: %s\n", cass_error_desc(rc));

    cass_statement_free(statement);
    cass_future_free(query_future);
    free(all_se);
}

void Cassandra::update_meta(char* file_name, unsigned char* tail_sk)
{
    CassStatement* statement
        = cass_statement_new("UPDATE meta SET tail_sk=? WHERE file_name=?", 2);

    cass_statement_bind_bytes(statement, 0, (const cass_byte_t*) tail_sk, 32);
    cass_statement_bind_string(statement, 1, file_name);

    CassFuture* query_future = cass_session_execute(session, statement);
    cass_statement_free(statement);
    cass_future_free(query_future);
}

std::vector<std::string> Cassandra::get_all_files()
{
    std::vector<std::string> r;
    CassStatement* statement = cass_statement_new("SELECT file_name FROM meta", 0);
    CassFuture* result_future = cass_session_execute(session, statement);
    if(cass_future_error_code(result_future) == CASS_OK) 
    {
        const CassResult* result = cass_future_get_result(result_future);
        CassIterator* rows = cass_iterator_from_result(result);
        while (cass_iterator_next(rows)) 
        {
             const CassRow* row = cass_iterator_get_row(rows);
             const CassValue* value = cass_row_get_column_by_name(row, "file_name");
	     const char* s;
	     size_t size;
             cass_value_get_string(value, &s, &size);
	     std::string str((char*) s, size);
             r.push_back(s);
	}
    }
    return r;
}

void Cassandra::get_meta(char* file_name, int* blocks_count, int* se_blocks_count,
        unsigned char** tail_fk, unsigned char** tail_sk, unsigned char* tails_se[32], unsigned char** tail_sgx)
{
    //printf("Reading meta... \n");
    CassStatement* statement = cass_statement_new("SELECT blocks_count, se_blocks_count, tail_fk, tail_sk, tail_se, tail_sgx FROM meta WHERE file_name=?", 1);
    cass_statement_bind_string(statement, 0, file_name);
    CassFuture* result_future = cass_session_execute(session, statement);
    if(cass_future_error_code(result_future) == CASS_OK) 
    {
        const CassResult* result = cass_future_get_result(result_future);
        CassIterator* rows = cass_iterator_from_result(result);
        if (cass_iterator_next(rows)) 
        {
             const CassRow* row = cass_iterator_get_row(rows);
             const CassValue* value;
             const cass_byte_t* bytes;
             size_t size;

             value = cass_row_get_column_by_name(row, "blocks_count");
             cass_value_get_int32(value, blocks_count);
             value = cass_row_get_column_by_name(row, "se_blocks_count");
             cass_value_get_int32(value, se_blocks_count);
             value = cass_row_get_column_by_name(row, "tail_fk");
             cass_value_get_bytes(value, &bytes, &size);
             *tail_fk = (unsigned char*) malloc(size);
             memcpy(*tail_fk, bytes, size);
             value = cass_row_get_column_by_name(row, "tail_sk");
             cass_value_get_bytes(value, &bytes, &size);
             *tail_sk = (unsigned char*) malloc(size);
             memcpy(*tail_sk, bytes, size);

             value = cass_row_get_column_by_name(row, "tail_se");
             cass_value_get_bytes(value, &bytes, &size);
             unsigned char* bytes_se = (unsigned char*) bytes;
             for(int i=0; i<*se_blocks_count; i++)
             {
                 tails_se[i] = (unsigned char*) malloc(32);
                 memcpy(tails_se[i], bytes_se, 32);
                 bytes_se += 32;
             }

             value = cass_row_get_column_by_name(row, "tail_sgx");
             cass_value_get_bytes(value, &bytes, &size);
             *tail_sgx = (unsigned char*) malloc(size);
             memcpy(*tail_sgx, bytes, size);
         }
         cass_result_free(result);
         cass_iterator_free(rows);
    }
    cass_statement_free(statement);
}

void Cassandra::insert_block(char* block_name, unsigned char* data, size_t size)
{
    CassStatement* statement
        = cass_statement_new("INSERT INTO blocks (block_name, data, size) VALUES (?, ?, ?)", 3);

    cass_statement_bind_string(statement, 0, block_name);
    cass_statement_bind_bytes(statement, 1, (const cass_byte_t*) data, size);
    cass_statement_bind_int32(statement, 2, size);

    CassFuture* query_future = cass_session_execute(session, statement);
    cass_statement_free(statement);
    cass_future_free(query_future);
}

void Cassandra::update_block(char* block_name, unsigned char* data, size_t size)
{
    CassStatement* statement
        = cass_statement_new("UPDATE blocks SET data=? WHERE block_name=?", 2);

    cass_statement_bind_bytes(statement, 0, (const cass_byte_t*) data, size);
    cass_statement_bind_string(statement, 1, block_name);

    CassFuture* query_future = cass_session_execute(session, statement);
    cass_statement_free(statement);
    cass_future_free(query_future);
}

void Cassandra::get_block(char* block_name, unsigned char** data, size_t* data_size)
{
    //printf("Reading block ...\n");
    CassStatement* statement = cass_statement_new("SELECT size, data FROM blocks WHERE block_name=?", 1);
    cass_statement_bind_string(statement, 0, block_name);
    CassFuture* result_future = cass_session_execute(session, statement);
    if(cass_future_error_code(result_future) == CASS_OK) 
    {
        const CassResult* result = cass_future_get_result(result_future);
        CassIterator* rows = cass_iterator_from_result(result);
        if (cass_iterator_next(rows)) 
        {
             const CassRow* row = cass_iterator_get_row(rows);
             const CassValue* value = cass_row_get_column_by_name(row, "data");
             const cass_byte_t* bytes;
             cass_value_get_bytes(value, &bytes, data_size);
             *data = (unsigned char*) malloc(*data_size);
             memcpy(*data, bytes, *data_size);
         }
         cass_result_free(result);
         cass_iterator_free(rows);
    }
    cass_statement_free(statement);
}

