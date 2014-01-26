//
//  tester.h
//  esb
//
//  Created by Evan Andersen on 12-12-14.
//  Copyright (c) 2012 Evan Andersen. All rights reserved.
//

#ifndef esb_tester_h
#define esb_tester_h

#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>

#include "global.h"
#include "storage_protocol.h"

#define kClientAddKeys      11
#define kClientRemoveKeys   12
#define kClientRunWorkload  13

#define kNumLatencyBuckets 524288

struct workerTask
{
    uint8_t type;
    uint16_t port;
    uint32_t connOpenDelay;
    uint32_t workerID;
    uint64_t count;
    uint64_t* latencyResults;
    uint64_t startingKey;
    uint64_t numKeys;
	uint64_t *keySecret; 		//128 bits for each secret
	uint64_t *valueSecret;
    double throughput;
    const char* table;
    void* conn;
    const char* hostname;
    const char* username;
    const char* password;
};




const char* testClient(void* parameters);


#endif
