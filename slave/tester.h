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

#define kWorkloadTypes 2

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
    uint32_t queryDensity;
    uint64_t count;
    uint64_t* latencyResults;
    uint64_t startingKey;
    uint64_t numKeys;
    double throughput;
    double workloadComposition[kWorkloadTypes]; //read, write (need n-1 chances, as they always add to 100%)
    const char* table;
    void* conn;
    const char* hostname;
    const char* username;
    const char* password;
};




void* testClient(void* parameters);


#endif
