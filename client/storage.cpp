#include "storage.h"

#include <fstream>
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem::v1;

static std::string path = "/home/stefan/code/aont/client/tmp_storage/";

#include "redis.h"

void write_to_storage(std::string key, unsigned char* value, size_t size)
{
    RedisCloud::PutBinary(key, value, size);
    return;

    std::string full_key_name = path + key;
    std::ofstream s(full_key_name);
    s.write(reinterpret_cast<const char*>(value), size);
    s.close();
}

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
    s.close();
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
