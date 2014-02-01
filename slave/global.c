//
//  global.c
//  esb
//
//  Created by Evan Andersen on 12-12-14.
//  Copyright (c) 2012 Evan Andersen. All rights reserved.
//

#include "global.h"


void* __xmalloc(size_t size)
{
    void* result = malloc(size);
    if(!result)
    {
        if(!size)
        {
            result = malloc(1);
        }
        if(!result)
        {
            fprintf(stderr, "Error allocating memory of size %zu\n", size);
            exit(-1);
        }
    }
    return result;
}

void* __xrealloc(void* pointer, size_t size)
{
    void* newPointer = realloc(pointer, size);
    if(!newPointer && !size)
    {
        fprintf(stderr, "Error re-allocating memory of size %zu\n", size);
        exit(-1);
    }
    return newPointer;
}


//return the number of 10uS ticks
uint64_t timeElapsed(struct timeval* start)
{
    return uSElapsed(start)/10;
}

int64_t uSElapsed(struct timeval* start)
{
    struct timeval end;
    gettimeofday(&end, NULL);
    return (end.tv_sec - start->tv_sec) * 1000000 + (end.tv_usec - start->tv_usec);
}

void hmac_sha256(const unsigned char *text, size_t text_len, const unsigned char *key, size_t key_len, void *digest)
{
    unsigned char k_ipad[65];   /* inner padding -
                                 * key XORd with ipad
                                 */
    unsigned char k_opad[65];   /* outer padding -
                                 * key XORd with opad
                                 */
    unsigned char tk[SHA256_DIGEST_LENGTH];
    unsigned char tk2[SHA256_DIGEST_LENGTH];
    unsigned char bufferIn[1024];
    unsigned char bufferOut[1024];
    int           i;
 
    /* if key is longer than 64 bytes reset it to key=sha256(key) */
    if (key_len > 64) {
        SHA256(key, key_len, tk);
        key     = tk;
        key_len = SHA256_DIGEST_LENGTH;
    }
 
    /*
     * the HMAC_SHA256 transform looks like:
     *
     * SHA256(K XOR opad, SHA256(K XOR ipad, text))
     *
     * where K is an n byte key
     * ipad is the byte 0x36 repeated 64 times
     * opad is the byte 0x5c repeated 64 times
     * and text is the data being protected
     */
 
    /* start out by storing key in pads */
    memset(k_ipad, 0, sizeof k_ipad);
    memset(k_opad, 0, sizeof k_opad);
    memcpy(k_ipad, key, key_len);
    memcpy(k_opad, key, key_len);
 
    /* XOR key with ipad and opad values */
    for (i = 0; i < 64; i++) {
        k_ipad[i] ^= 0x36;
        k_opad[i] ^= 0x5c;
    }
 
    /*
     * perform inner SHA256
     */
    memset(bufferIn, 0x00, 1024);
    memcpy(bufferIn, k_ipad, 64);
    memcpy(bufferIn + 64, text, text_len);
 
    SHA256(bufferIn, 64 + text_len, tk2);
 
    /*
     * perform outer SHA256
     */
    memset(bufferOut, 0x00, 1024);
    memcpy(bufferOut, k_opad, 64);
    memcpy(bufferOut + 64, tk2, SHA256_DIGEST_LENGTH);
 
    SHA256(bufferOut, 64 + SHA256_DIGEST_LENGTH, digest);
}

void stringGen(uint64_t key_index, uint64_t *secret, char *out, size_t out_len) 
{
	unsigned char digest[32];
	hmac_sha256((unsigned char*)&key_index, sizeof(key_index), (unsigned char*)secret, sizeof(secret), digest);
	
	if(out_len > 65) {
		out_len = 65;
	}
	
    const char* hex = "0123456789abcdef";
	for(size_t i = 0; i < (out_len/2); i++) {
		out[i*2    ] = hex[digest[i] & 0xf];
		out[i*2 + 1] = hex[digest[i] >> 4];
	}
	out[out_len - 1] = '\0';
}

const char* ece297strerror(int error)
{
    switch (error)
    {
        case ERR_INVALID_PARAM:
        {
            return "Invalid parameters";
        }
        case ERR_CONNECTION_FAIL:
        {
            return "Failed to connect to server";
        }
        case ERR_NOT_AUTHENTICATED:
        {
            return "Not authenticated";
        }
        case ERR_AUTHENTICATION_FAILED:
        {
            return "Invalid username or password";
        }
        case ERR_TABLE_NOT_FOUND:
        {
            return "Table not found";
        }
        case ERR_KEY_NOT_FOUND:
        {
            return "Key not found";
        }
        default:
        {
            return "Unknown error";
        }
    }
}


