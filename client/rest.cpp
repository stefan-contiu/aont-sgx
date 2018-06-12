#include "rest.h"
#include <string.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <curl/curlbuild.h>

char* rest_address = (char*)"http://192.168.1.111:5000";

int rest_write(std::string operation, std::string key, unsigned char* data, size_t size);
int rest_read(std::string operation, std::string key, unsigned char** pdata, size_t* psize);

int rest_write_metadata(std::string key, unsigned char* data, size_t size)
{
    return rest_write("write_metadata", key, data, size);
}

int rest_write_block(std::string key, unsigned char* data, size_t size)
{
    return rest_write("write_block", key, data, size);
}

int rest_read_metadata(std::string key, unsigned char** pdata, size_t* psize)
{
    return rest_read("read_metadata", key, pdata, psize);
}

int rest_read_block(std::string key, unsigned char** pdata, size_t* psize)
{
    return rest_read("read_block", key, pdata, psize);
}

int rest_write(std::string operation, std::string key, unsigned char* data, size_t size)
{
    std::string url(rest_address);
    url.append("/");
    url.append(operation);
    url.append("/");
    url.append(key);
    CURL* curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, size);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return 0;
}

struct string {
  char *ptr;
  size_t len;
};

void init_string(struct string *s) {
  s->len = 0;
  s->ptr = (char*) malloc(s->len+1);
  if (s->ptr == NULL) {
    fprintf(stderr, "malloc() failed\n");
    exit(EXIT_FAILURE);
  }
  s->ptr[0] = '\0';
}

size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s)
{
  size_t new_len = s->len + size*nmemb;
  s->ptr = (char*) realloc(s->ptr, new_len+1);
  if (s->ptr == NULL) {
    fprintf(stderr, "realloc() failed\n");
    exit(EXIT_FAILURE);
  }
  memcpy(s->ptr+s->len, ptr, size*nmemb);
  s->ptr[new_len] = '\0';
  s->len = new_len;

  return size*nmemb;
}

int rest_read(std::string operation, std::string key, unsigned char** pdata, size_t* psize)
{
    std::string url(rest_address);
    url.append("/");
    url.append(operation);
    url.append("/");
    url.append(key);
    CURL* curl = curl_easy_init();

    struct string s;
    init_string(&s);

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    *pdata = (unsigned char*) malloc(s.len);
    memcpy(*pdata, s.ptr, s.len);
    *psize = s.len;
    return 0;
}
