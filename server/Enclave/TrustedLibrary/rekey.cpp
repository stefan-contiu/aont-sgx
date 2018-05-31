/*
 * Copyright (C) 2011-2017 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


#include <string.h>
#include "sgx_cpuid.h"

#include "sgx_trts.h"
#include "../Enclave.h"
#include "Enclave_t.h"

#include "tSgxSSL_api.h"
#include "tsgxsslio.h"

#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <openssl/evp.h>

#include <string>

#include "sgx_tcrypto.h"

/* ecall_malloc_free:
 *   Uses malloc/free to allocate/free trusted memory.
 */
void ecall_malloc_free(void)
{
    void *ptr = malloc(100);
    assert(ptr != NULL);
    memset(ptr, 0x0, 100);
    free(ptr);
}

unsigned char* old_gk = (unsigned char*) "12345678901234567890123456789012";
unsigned char* new_gk = (unsigned char*) "00001111222233334444455556789012";

void pack32(uint32_t val,uint8_t *dest)
{
        dest[0] = (val & 0xff000000) >> 24;
        dest[1] = (val & 0x00ff0000) >> 16;
        dest[2] = (val & 0x0000ff00) >>  8;
        dest[3] = (val & 0x000000ff)      ;
}

uint32_t unpack32(uint8_t *src)
{
        uint32_t val;

        val  = src[0] << 24;
        val |= src[1] << 16;
        val |= src[2] <<  8;
        val |= src[3]      ;

        return val;
}

void long_to_byte_array(unsigned long n, unsigned char* bytes, size_t bytes_size)
{
    memset(bytes, 0, bytes_size);
    bytes[0] = (n >> 24) & 0xFF;
    bytes[1] = (n >> 16) & 0xFF;
    bytes[2] = (n >> 8) & 0xFF;
    bytes[3] = n & 0xFF;
}

void byte_array_to_long(unsigned char* bytes, unsigned long* n)
{
    *n = ((bytes[0] << 24)
         +(bytes[1] << 16)
         +(bytes[2] << 8)
         +(bytes[3]));
}


static inline void print_hex(unsigned char *h, int l)
{
    for (int i=0; i<l; i++)
        printf("%02X", h[i]);
    printf("\n");
}


void deserialize_metadata_stream(
    unsigned char* inputStream,
    int inputStreamSize,
    int* p_blocks_count,
    int* p_enc_oeb_index_size,
    unsigned char** tail_fk, // 32 bytes
    unsigned char** tail_oeb, // 32 bytes
    unsigned char** enc_oeb_index, // enc_oeb_index_size bytes
    unsigned char** iv)
{
    *p_blocks_count = unpack32(inputStream);
    inputStream += 4;
    *p_enc_oeb_index_size = unpack32(inputStream);
    inputStream += 4;

    (*tail_fk) = (unsigned char*) malloc(32);
    (*tail_oeb) = (unsigned char*) malloc(32);
    (*enc_oeb_index) = (unsigned char*) malloc(*p_enc_oeb_index_size);
    (*iv) = (unsigned char*) malloc(32);

    memcpy(*tail_fk, inputStream, 32);
    inputStream += 32;

    memcpy(*tail_oeb, inputStream, 32);
    inputStream += 32;

    memcpy(*enc_oeb_index, inputStream, *p_enc_oeb_index_size);
    inputStream += *p_enc_oeb_index_size;

    memcpy(*iv, inputStream, 32);
    inputStream += 32;
}


void serialize_metadata_to_stream(
    unsigned char** meta_stream,
    int* p_meta_stream_size,
    int blocks_count,
    int enc_oeb_index_size,
    unsigned char* tail_fk, // 32 bytes
    unsigned char* tail_oeb, // 32 bytes
    unsigned char* enc_oeb_index, // enc_oeb_index_size bytes
    unsigned char* iv
)
{
    int stream_size = 4 + 4 + 32 + 32 + 32 + enc_oeb_index_size;
    *meta_stream = (unsigned char*) malloc(stream_size);
    int stream_index = 0;

    unsigned char bc[4];
    pack32(blocks_count, bc);
    memcpy(*meta_stream + stream_index, bc, 4);
    stream_index += 4;

    unsigned char bi[4];
    pack32(enc_oeb_index_size, bi);
    memcpy(*meta_stream + stream_index, bi, 4);
    stream_index += 4;

    memcpy(*meta_stream + stream_index, tail_fk, 32);
    stream_index += 32;
    memcpy(*meta_stream + stream_index, tail_oeb, 32);
    stream_index += 32;
    memcpy(*meta_stream + stream_index, enc_oeb_index, enc_oeb_index_size);
    stream_index += enc_oeb_index_size;
    memcpy(*meta_stream + stream_index, iv, 32);
    stream_index += 32;

    *p_meta_stream_size = stream_index;
}

void test_serialization()
{
    // serialize
    unsigned char* tail_fk = (unsigned char*) "12345678901234567890123456789012";
    unsigned char* tail_oeb = (unsigned char*) "XXX45678901234567890123456789YYY";
    unsigned char* iv = (unsigned char*) "11122233301234567890123456789999";
    unsigned char* enc_oeb_index = (unsigned char*) malloc(4096);
    unsigned char* m;
    int m_size;
    int o_b1 = 123459;
    int o_b2 = 4096;
    serialize_metadata_to_stream(&m, &m_size, o_b1, o_b2, tail_fk, tail_oeb,
        enc_oeb_index, iv);
    print_hex(m, 64);

    // deserialiaze
    int b1;
    int b2;
    unsigned char* d_tail_fk;
    unsigned char* d_tail_oeb;
    unsigned char* d_enc_oeb_index;
    unsigned char* d_iv;
    deserialize_metadata_stream(m, m_size, &b1, &b2, &d_tail_fk, &d_tail_oeb, &d_enc_oeb_index, &d_iv);

    // verify
    printf("Blocks Count : %d vs %d\n", o_b1, b1);
    printf("OBE B size   : %d vs %d\n", o_b2, b2);
    //print_hex(tail_fk, 32);
    //print_hex(d_tail_fk, 32);
}

static char rsaPrivateKey[]="-----BEGIN RSA PRIVATE KEY-----\n"\
"MIIEowIBAAKCAQEAy8Dbv8prpJ/0kKhlGeJYozo2t60EG8L0561g13R29LvMR5hy\n"\
"vGZlGJpmn65+A4xHXInJYiPuKzrKUnApeLZ+vw1HocOAZtWK0z3r26uA8kQYOKX9\n"\
"Qt/DbCdvsF9wF8gRK0ptx9M6R13NvBxvVQApfc9jB9nTzphOgM4JiEYvlV8FLhg9\n"\
"yZovMYd6Wwf3aoXK891VQxTr/kQYoq1Yp+68i6T4nNq7NWC+UNVjQHxNQMQMzU6l\n"\
"WCX8zyg3yH88OAQkUXIXKfQ+NkvYQ1cxaMoVPpY72+eVthKzpMeyHkBn7ciumk5q\n"\
"gLTEJAfWZpe4f4eFZj/Rc8Y8Jj2IS5kVPjUywQIDAQABAoIBADhg1u1Mv1hAAlX8\n"\
"omz1Gn2f4AAW2aos2cM5UDCNw1SYmj+9SRIkaxjRsE/C4o9sw1oxrg1/z6kajV0e\n"\
"N/t008FdlVKHXAIYWF93JMoVvIpMmT8jft6AN/y3NMpivgt2inmmEJZYNioFJKZG\n"\
"X+/vKYvsVISZm2fw8NfnKvAQK55yu+GRWBZGOeS9K+LbYvOwcrjKhHz66m4bedKd\n"\
"gVAix6NE5iwmjNXktSQlJMCjbtdNXg/xo1/G4kG2p/MO1HLcKfe1N5FgBiXj3Qjl\n"\
"vgvjJZkh1as2KTgaPOBqZaP03738VnYg23ISyvfT/teArVGtxrmFP7939EvJFKpF\n"\
"1wTxuDkCgYEA7t0DR37zt+dEJy+5vm7zSmN97VenwQJFWMiulkHGa0yU3lLasxxu\n"\
"m0oUtndIjenIvSx6t3Y+agK2F3EPbb0AZ5wZ1p1IXs4vktgeQwSSBdqcM8LZFDvZ\n"\
"uPboQnJoRdIkd62XnP5ekIEIBAfOp8v2wFpSfE7nNH2u4CpAXNSF9HsCgYEA2l8D\n"\
"JrDE5m9Kkn+J4l+AdGfeBL1igPF3DnuPoV67BpgiaAgI4h25UJzXiDKKoa706S0D\n"\
"4XB74zOLX11MaGPMIdhlG+SgeQfNoC5lE4ZWXNyESJH1SVgRGT9nBC2vtL6bxCVV\n"\
"WBkTeC5D6c/QXcai6yw6OYyNNdp0uznKURe1xvMCgYBVYYcEjWqMuAvyferFGV+5\n"\
"nWqr5gM+yJMFM2bEqupD/HHSLoeiMm2O8KIKvwSeRYzNohKTdZ7FwgZYxr8fGMoG\n"\
"PxQ1VK9DxCvZL4tRpVaU5Rmknud9hg9DQG6xIbgIDR+f79sb8QjYWmcFGc1SyWOA\n"\
"SkjlykZ2yt4xnqi3BfiD9QKBgGqLgRYXmXp1QoVIBRaWUi55nzHg1XbkWZqPXvz1\n"\
"I3uMLv1jLjJlHk3euKqTPmC05HoApKwSHeA0/gOBmg404xyAYJTDcCidTg6hlF96\n"\
"ZBja3xApZuxqM62F6dV4FQqzFX0WWhWp5n301N33r0qR6FumMKJzmVJ1TA8tmzEF\n"\
"yINRAoGBAJqioYs8rK6eXzA8ywYLjqTLu/yQSLBn/4ta36K8DyCoLNlNxSuox+A5\n"\
"w6z2vEfRVQDq4Hm4vBzjdi3QfYLNkTiTqLcvgWZ+eX44ogXtdTDO7c+GeMKWz4XX\n"\
"uJSUVL5+CVjKLjZEJ6Qc2WZLl94xSwL71E41H4YciVnSCQxVc4Jw\n"\
"-----END RSA PRIVATE KEY-----\n";

void xor_into_array(unsigned char* src, unsigned char* array_to_xor, int size)
{
    for(int i=0; i<size; i++)
    {
        src[i] ^= array_to_xor[i];
    }
}

int rsa_decryption(
    unsigned char* ciphertext, int ciphertext_length,
    char* key, int key_length,
    unsigned char* plaintext)
{
    OPENSSL_init_crypto(0, NULL);
    BIO *bio_buffer = NULL;
    RSA *rsa = NULL;

    bio_buffer = BIO_new_mem_buf((void*)key, key_length);

    PEM_read_bio_RSAPrivateKey(bio_buffer, &rsa, 0, NULL);

    int plaintext_length = RSA_private_decrypt(
        ciphertext_length,
        ciphertext,
        plaintext,
        rsa,
        RSA_PKCS1_PADDING);

    return plaintext_length;
}

void sgx_aes_encrypt(
    unsigned char* plaintext,
    int plaintext_size,
    unsigned char* key, unsigned char* iv,
    unsigned char* ciphertext)
{
    int len;
    int ciphertext_len;
    EVP_CIPHER_CTX *ctx;
    ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_ctr(), NULL, key, iv);
    EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_size);
    ciphertext_len = len;
    EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
    ciphertext_len += len;
    EVP_CIPHER_CTX_free(ctx);
}

void sgx_aes_decrypt(
    unsigned char* ciphertext,
    int ciphertext_len,
    unsigned char* key, unsigned char* iv,
    unsigned char* plaintext)
{
//    printf("\n CIPHER : "); print_hex(ciphertext, 128);
//    printf("\n KEY : "); print_hex(key, 32);
//    printf("\n IV : "); print_hex(iv, 32);

    EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_len;
    ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_ctr(), NULL, key, iv);
    EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len);
    plaintext_len = len;
    EVP_DecryptFinal_ex(ctx, (plaintext) + len, &len);
    plaintext_len += len;
    EVP_CIPHER_CTX_free(ctx);
}

unsigned char* sgx_sha256(const unsigned char *d,
    size_t n,
    unsigned char *md)
{
    return SHA256(d, n, md);
}

/*****************************************************************************
TEST CODE FOR SRDS2018
******************************************************************************/

// assume that there exist a 128 bits symmetric key priorly provisioned to the enclaves
char* sgx_provisioned_key = (char*)"1234567890123456";

void set_ctr_bytes(uint32_t val, uint8_t *ctr, size_t ctr_size)
{
	// within our simulation counters do not exceed 2^32 values, meaning that they can
	// be represented on 4 bytes.
	ctr[ctr_size - 4] = (val & 0xff000000) >> 24;
	ctr[ctr_size - 3] = (val & 0x00ff0000) >> 16;
	ctr[ctr_size - 2] = (val & 0x0000ff00) >>  8;
	ctr[ctr_size - 1] = (val & 0x000000ff);
}

sgx_status_t decryptMessage(char* in, size_t in_size, char* out, uint32_t counter)
{
	uint8_t ctr_bytes[16] = {0};
	set_ctr_bytes(counter, ctr_bytes, 16);
	return sgx_aes_ctr_decrypt((sgx_aes_ctr_128bit_key_t*) sgx_provisioned_key,
		(uint8_t*) in, in_size, ctr_bytes, 128,
		(uint8_t*) out);
}

sgx_status_t encryptMessage(char* in, size_t in_size, char* out, uint32_t counter)
{
	uint8_t ctr_bytes[16] = {0};
	set_ctr_bytes(counter, ctr_bytes, 16);
	return sgx_aes_ctr_encrypt(
		(sgx_aes_ctr_128bit_key_t*) sgx_provisioned_key,
		(uint8_t*) in, in_size, ctr_bytes, 128,
		(uint8_t*) out);
}

void send_by_socket(unsigned char* data, size_t size)
{
    // ...
}

void test_encrypt()
{
    // simulate a large video segment
    int segment_size = 1000;
    unsigned char segment[segment_size];
    sgx_read_rand(segment, segment_size);

    unsigned char* previous_subpacket_tail;
    int previous_subpacket_tail_size = 0;

    // split the segment in arbitrary size sub-packets
    int segment_offset = 0;
    int counter_16bytes = 0;
    while(segment_offset < segment_size)
    {
        // --- simulate the recv of random size sub-packet
        unsigned char sub_packet_size;
        sgx_read_rand(&sub_packet_size, 1);
        // make sure we don't overflow the segment size
        if (segment_offset + sub_packet_size > segment_size)
        {
            sub_packet_size = segment_size - segment_offset;
        }
        // see here that we allocate space for the subpacket and the tail of the previous sub-packet
        unsigned char* sub_packet = (unsigned char*) malloc(previous_subpacket_tail_size + sub_packet_size);
        printf("Received sub-packet size : %d\n", (int) sub_packet_size);
        memcpy(sub_packet + previous_subpacket_tail_size, segment + segment_offset, sub_packet_size);

        // pre-pend to the sub-packet the tail of previous sub-packet
        memcpy(sub_packet, previous_subpacket_tail, previous_subpacket_tail_size);
        sub_packet_size += previous_subpacket_tail_size;
        printf("  * prepended previous tail. new sub-packet size : %d\n", (int) sub_packet_size);

        // trim the tail of sub-packet (e.g. tail = whatever overflows from the last multiple of 16)
        int valid_packet_size = 16 * (sub_packet_size / 16);
        printf("  * packet is trimmed to multiple of 16. new size : %d\n", (int) valid_packet_size);

        // encrypt and send data
        unsigned char out[valid_packet_size];
        encryptMessage((char*) sub_packet, valid_packet_size, (char*) out, counter_16bytes);
        send_by_socket(out, valid_packet_size);

        // increment counter
        counter_16bytes += valid_packet_size / 16;
        printf("  * aes-ctr counter incremented to value : %d\n", counter_16bytes);

        // retain tail for the next iteration
        previous_subpacket_tail_size = sub_packet_size - valid_packet_size;
        previous_subpacket_tail = (unsigned char*) malloc(previous_subpacket_tail_size);
        memcpy(previous_subpacket_tail, sub_packet + valid_packet_size, previous_subpacket_tail_size);
        printf("  * the new tail has size : %d\n", previous_subpacket_tail_size);

        // move the offset in the video segment
        segment_offset += sub_packet_size;
    }

    // if there is any tail leftover :
    if (previous_subpacket_tail_size > 0)
    {
        unsigned char out[previous_subpacket_tail_size];
        encryptMessage((char*) previous_subpacket_tail, previous_subpacket_tail_size,
            (char*) out, counter_16bytes);
        send_by_socket(out, previous_subpacket_tail_size);
    }
}


void test_srds()
{
    printf("--- TEST SRDS Encryption \n");
    test_encrypt();
    return;
/*
    char* p = "Of recommend residence education be on difficult repulsive offending. Judge views had mirth table seems great him for her. Alone all happy asked begin fully stand own get. Excuse ye seeing result of we. See scale dried songs old may not. Promotion did disposing you household any instantly. Hills we do under times at first short an.";
    int p_size = strlen(p);
    char* c = (char*) malloc(p_size);

    // encrypt a buffer
    sgx_status_t enc_result = encrypt_ctr(p, p_size, c);
    if (enc_result != SGX_SUCCESS)
    {
        printf("Encryption error in SGX.\n");
        return;
    }

    // test 1 : decrypt the whole buffer
    char* d = (char*) malloc(p_size);
    sgx_status_t dec_result = decrypt_ctr(c, p_size, 0, d);
    if (dec_result == SGX_SUCCESS)
    {
        printf("Test 1 passed. Plaintext : \n%s\n", d);
    }
    else
    {
        printf("Decryption error in SGX.\n");
    }

    // test 2 : decrypt pieces of 12 bytes from the buffer
    int chunk_size = 48;
    int offset = 0;
    while(offset < p_size)
    {
        char* chunk = (char*) malloc(chunk_size + 1);
        decrypt_ctr(c + offset, chunk_size, offset / 16, chunk);
        offset += chunk_size;

        {
            // print message
            if (offset > p_size)
            {
                chunk[p_size % chunk_size] = 0;
            }
            else
            {
                chunk[chunk_size] = 0;
            }
            printf("Chunk : %s\n", chunk);
        }

        free(chunk);
    }

    // clean-up
    free(c);
    free(d);
    */
}


/*****************************************************************************
******************************************************************************/

void worker_loop()
{
    while(true)
    {
        //
    }
}

void slave_re_key(char* buff)
{
    // deserialize slave request
    unsigned char old_gk[32];
    unsigned char new_gk[32];
    unsigned char iv[32];

    // split the input buffer in
}

void ecall_worker_re_key(
    char* key,
    size_t key_size,
    char* metadata,
    size_t meta_size)
{
    test_srds();
    return;

    //printf("--- HELLO ECALL-REKEY-WORKER\n");

    int blocks_count;
    int enc_oeb_index_size;
    unsigned char* tail_fk;
    unsigned char* tail_bi;
    unsigned char* enc_oeb_index;
    unsigned char* iv;

    deserialize_metadata_stream((unsigned char*)metadata, meta_size,
        &blocks_count,
        &enc_oeb_index_size,
        &tail_fk, // 32 bytes
        &tail_bi, // 32 bytes
        &enc_oeb_index, // enc_oeb_index_size bytes
        &iv);

    //printf("SGX says %d blocks\n", blocks_count);
    //printf("SGX says %d enc_oeb_index_size\n", enc_oeb_index_size);
    //print_hex(enc_oeb_index, 256);

    // decrypt OEB index
    long unsigned oeb;
    unsigned char* dec_oeb_index = (unsigned char*) malloc(4096);
    rsa_decryption(enc_oeb_index, enc_oeb_index_size,
        rsaPrivateKey, strlen(rsaPrivateKey),
        dec_oeb_index);
    byte_array_to_long(dec_oeb_index, &oeb);
    printf("SGX says SE Block Index %d\n", (int)oeb);

    // bring the over-encrypted block in the enclave (OCALL)
    std::string blockName = std::string(key, key_size);
    //printf("SGX blockName initial : %s\n", blockName.c_str());
    // strip .metadata suffix and add block index
    blockName.replace(blockName.size() - 9, 9, "");
    blockName = blockName + "." + std::to_string(oeb);

    unsigned char* enc_block = (unsigned char*) malloc(1024 * 256 * 4);
    int block_size = 0;
    int s;
    ocall_get_block(&block_size, (char*)blockName.c_str(), &enc_block, s);
    //printf("SGX Block returned from OCALL size : %d\n", block_size);
    //print_hex(enc_block, 32);

    // decrypt it by using old_gk
    unsigned char* dec_enc_block = (unsigned char*) malloc(block_size);
    sgx_aes_decrypt(enc_block, block_size, old_gk, iv, dec_enc_block);

    // encrypt it by using new_gk
    unsigned char* enc_enc_block = (unsigned char*) malloc(block_size);
    sgx_aes_encrypt(dec_enc_block, block_size, new_gk, iv, enc_enc_block);

    // push it back to the storage (OCALL)
    ocall_put_block((char*)blockName.c_str(), enc_enc_block, block_size);


    // encrypt the tail by using the group_key
    unsigned char* dec_tail_fk = (unsigned char*) malloc(32);
    unsigned char* dec_tail_bi = (unsigned char*) malloc(32);
    sgx_aes_decrypt(tail_fk, 32, old_gk, iv, dec_tail_fk);
    sgx_aes_decrypt(tail_bi, 32, old_gk, iv, dec_tail_bi);

    // adjust the tail_bi, xor with the old and new block hash
    unsigned char* enc_block_sha = (unsigned char*) malloc(32);
    sgx_sha256(enc_block, block_size, enc_block_sha);
    xor_into_array(dec_tail_bi, enc_block_sha, 32);

    unsigned char* enc_enc_block_sha = (unsigned char*) malloc(32);
    sgx_sha256(enc_enc_block, block_size, enc_enc_block_sha);
    xor_into_array(dec_tail_bi, enc_enc_block_sha, 32);

    unsigned char* new_tail_fk = (unsigned char*) malloc(32);
    unsigned char* new_tail_bi = (unsigned char*) malloc(32);
    sgx_aes_encrypt(dec_tail_fk, 32, new_gk, iv, new_tail_fk);
    sgx_aes_encrypt(dec_tail_bi, 32, new_gk, iv, new_tail_bi);
    printf("TAIL BI : "); print_hex(new_tail_bi, 32);

    // metadata file: blocks count, tail_fk (32), tail_bk (32), enc_oeb_index, iv (32)
    std::string meta_file_name = blockName + ".metadata";
    unsigned char* meta_stream;
    int meta_stream_size;
    //printf("Blocks Count %d\n", blocks_count);
    serialize_metadata_to_stream(
        &meta_stream,
        &meta_stream_size,
        blocks_count,
        enc_oeb_index_size,
        new_tail_fk, // 32 bytes
        new_tail_bi, // 32 bytes
        enc_oeb_index, // enc_oeb_index_size bytes
        iv);

    // push back metadata to the storage
    ocall_put_block(key, meta_stream, meta_stream_size);

    //printf("--- BYE ECALL-REKEY-WORKER\n");

    // TODO : free up some of the space
    free(dec_enc_block);
    free(enc_enc_block);
}

/* ecall_sgx_cpuid:
 *   Uses sgx_cpuid to get CPU features and types.
 */
void ecall_sgx_cpuid(int cpuinfo[4], int leaf)
{
    test_serialization();

    printf("SGX STEFAN : hello !!!\n");
    RSA* rsa = RSA_new();
    printf("I DID NOT DIE !!!\n");

    sgx_status_t ret = sgx_cpuid(cpuinfo, leaf);
    if (ret != SGX_SUCCESS)
        abort();
}
