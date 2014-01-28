//
//  tester.c
//  esb
//
//  Created by Evan Andersen on 12-12-14.
//  Copyright (c) 2012 Evan Andersen. All rights reserved.
//

#include "tester.h"

static void recordLatency(uint64_t dataPoint, uint64_t* stats)
{
    if(dataPoint < kNumLatencyBuckets)
    {
        __sync_fetch_and_add(&stats[dataPoint], 1);
    }
}

const char* testClient(void* parameters)
{
    struct workerTask* worker = (struct workerTask*)parameters;
    
    uint16_t randContext[3];
    for(int i = 0; i < 3; i++)
    {
        randContext[i] = time(NULL) ^ worker->workerID;
    }
    
    if(!worker->conn)
    {
        //spread the load from new connections so the server won't be overloaded
        usleep(erand48(randContext) * 1000 + 1000*worker->connOpenDelay);
        
        worker->conn = storage_connect(worker->hostname, worker->port);
        if(!worker->conn)
        {
            printf("storage_connect failed\n");
            return ece297strerror(errno);
        }
        
        int result = storage_auth(worker->username, worker->password, worker->conn);
        if(result == -1)
        {
            printf("storage_auth failed\n");
            storage_disconnect(worker->conn);
            worker->conn = NULL;
            return ece297strerror(errno);
        }
    }
    
    uint64_t loopCount = worker->numKeys;
    if(worker->type == kClientRunWorkload)
    {
        loopCount = worker->count;
    }
    
    uint64_t period = 0;
    
    //0 throughput = no limit
    if(worker->throughput)
    {
        period = (1/worker->throughput) * 1000000;
        
        //start at a random time
        usleep(erand48(randContext) * period);
    }
    struct timeval next;
    gettimeofday(&next, NULL);
    
    for(uint64_t i = 0; i < loopCount; i++)
    {
        if(worker->throughput)
        {
            int64_t timeRemaining = -uSElapsed(&next);
            if(timeRemaining > 0)
            {
                usleep((uint32_t)timeRemaining);
            }
            uint64_t newTime = next.tv_usec + period;
            next.tv_sec += newTime / 1000000;
            next.tv_usec = newTime % 1000000;
        }
        switch (worker->type)
        {
            case kClientAddKeys:
            {
                char keyBuf[20]; //as per ECE297 spec
				stringGen(worker->startingKey + i, worker->keySecret, keyBuf, sizeof(keyBuf));
                
                struct storage_record record;
                memset(&record.metadata, 0, sizeof(record.metadata));
                
				stringGen(worker->startingKey + i, worker->valueSecret, record.value, sizeof(record.value));
                                				
                struct timeval start;
                gettimeofday(&start, NULL);
                if(storage_set(worker->table, keyBuf, &record, worker->conn) == -1)
                {
                    printf("storage_set failed\n");
                    return ece297strerror(errno);
                }
                recordLatency(timeElapsed(&start), worker->latencyResults);
                break;
            }
                                
            case kClientRunWorkload:
            {
                uint64_t keyIndex = (erand48(randContext) * worker->numKeys) + worker->startingKey;
                
                char keyBuf[20]; //as per ECE297 spec
				stringGen(keyIndex, worker->keySecret, keyBuf, sizeof(keyBuf));
                char expectedValue[1024];
				stringGen(keyIndex, worker->valueSecret, expectedValue, sizeof(expectedValue));
				
                struct timeval start;
                gettimeofday(&start, NULL);
                struct storage_record rec;
                if(storage_get(worker->table, keyBuf, &rec, worker->conn) == -1)
                {
                    printf("storage_get failed (key index = %u)\n", keyIndex);
                    return ece297strerror(errno);
                }
                if(strcmp(rec.value, expectedValue)) {
                	return "Server returned incorrect key";
                }
				
                recordLatency(timeElapsed(&start), worker->latencyResults);
            }
        }
    }
    return NULL;
}






