#ifndef STORAGE_H
#define STORAGE_H

#include <string>
#include <vector>

extern void write_to_storage(std::string key, unsigned char* value, size_t size);

extern void read_from_storage(std::string key, unsigned char** value, size_t* p_size);

/*
extern void write_to_redis(std::string key, unsigned char* value, size_t size);

extern void read_from_redis(std::string key, unsigned char** value, size_t* p_size);
*/

extern void get_all_metadata_keys(std::vector<std::string>& metadata_keys);

#endif
