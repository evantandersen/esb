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

void* testClient(void* parameters)
{
    //copy the worker parametrs onto the stack so we don't have to free them later
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
            return ece297strerror(errno);
        }
        
        int result = storage_auth(worker->username, worker->password, worker->conn);
        if(result == -1)
        {
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
                char keyBuf[128];
                hashInt64(worker->startingKey + i, keyBuf);
                
                struct storage_record record;
                memset(&record.metadata, 0, sizeof(record.metadata));
                
                //if error checking is enabled, copy that value in.
                if(worker->values)
                {
                    strcpy(record.value, worker->values[i]);
                }
                //otherwise, just generate a random string of the correct length
                else
                {
                    for(int j = 0; j < worker->valueSize; j++)
                    {
                        record.value[j] = (erand48(randContext) * 26) + 'A';
                    }
                    record.value[worker->valueSize] = '\0';
                }
                                
                struct timeval start;
                gettimeofday(&start, NULL);
                if(storage_set(worker->table, keyBuf, &record, worker->conn) == -1)
                {
                    return ece297strerror(errno);
                }
                recordLatency(timeElapsed(&start), worker->latencyResults);
                break;
            }
                
            case kClientRemoveKeys:
            {
                char keyBuf[128];
                hashInt64(worker->startingKey + i, keyBuf);

                struct timeval start;
                gettimeofday(&start, NULL);
                if(storage_set(worker->table, keyBuf, NULL, worker->conn) == -1)
                {
                    return ece297strerror(errno);
                }
                
                recordLatency(timeElapsed(&start), worker->latencyResults);
                break;
            }
                
            case kClientRunWorkload:
            {
                uint64_t keyIndex = erand48(randContext) * worker->numKeys;
                
                char keyBuf[128];
                hashInt64(keyIndex + worker->startingKey, keyBuf);
                
                double chance = erand48(randContext);
                
                //read
                if(chance < worker->workloadComposition[0])
                {
                    struct timeval start;
                    gettimeofday(&start, NULL);
                    struct storage_record rec;
                    if(storage_get(worker->table, keyBuf, &rec, worker->conn) == -1)
                    {
                        return ece297strerror(errno);
                    }
                    
                    recordLatency(timeElapsed(&start), worker->latencyResults);
                    
                    //check for accuracy
                    if(worker->values)
                    {
                        if(strcmp(worker->values[keyIndex], rec.value))
                        {
                            //values don't match
                            printf("Server returned %s, expected %s\n", rec.value, worker->values[keyIndex]);
                            return "Server returned incorrect key";
                        }
                    }
                }
                //write
                else
                {
                    struct storage_record rec;
                    memset(&rec.metadata, 0, sizeof(rec.metadata));
                    
                    for(int j = 0; j < worker->valueSize; j++)
                    {
                        rec.value[j] = (erand48(randContext) * 26) + 'A';
                    }
                    rec.value[worker->valueSize] = '\0';
                    
                    //if we are error checking, we need to change the value in memory
                    if(worker->values)
                    {
                        for(int j = 0; j < worker->valueSize; j++)
                        {
                            worker->values[keyIndex][j] = rec.value[j];
                        }
                    }
                                        
                    struct timeval start;
                    gettimeofday(&start, NULL);
                    if(storage_set(worker->table, keyBuf, &rec, worker->conn) == -1)
                    {
                        return ece297strerror(errno);
                    }
                    
                    recordLatency(timeElapsed(&start), worker->latencyResults);
                }
                break;
            }
        }
    }
    return NULL;
}