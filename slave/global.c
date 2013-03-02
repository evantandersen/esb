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


//return the number of 100uS ticks
uint64_t timeElapsed(struct timeval* start)
{
    struct timeval end;
    gettimeofday(&end, NULL);
    return (end.tv_sec - start->tv_sec) * 10000  + (end.tv_usec - start->tv_usec)/100;
}

int64_t uSElapsed(struct timeval* start)
{
    struct timeval end;
    gettimeofday(&end, NULL);
    return (end.tv_sec - start->tv_sec) * 1000000 + (end.tv_usec - start->tv_usec);
}

//20 char string to meet ECE297 spec
void hashInt64(uint64_t input, char* output)
{
    char* hex = "0123456789abcdef";
    MD5_CTX context;
    MD5Init(&context);
    MD5Update(&context, (unsigned char*)&input, 8);
    MD5Final(&context);
    for(int i = 0; i < 9; i++)
    {
        output[i*2    ] = hex[context.digest[i] & 0xf];
        output[i*2 + 1] = hex[context.digest[i] >> 4];
    }
    output[18] = hex[context.digest[9] & 0xf];
    output[19] = '\0';
}


