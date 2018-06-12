#ifndef REST_H
#define REST_H

#include <string>

int rest_write_metadata(std::string key, unsigned char* data, size_t size);
int rest_write_block(std::string key, unsigned char* data, size_t size);

int rest_read_metadata(std::string key, unsigned char** pdata, size_t* psize);
int rest_read_block(std::string key, unsigned char** data, size_t* size);

#endif
