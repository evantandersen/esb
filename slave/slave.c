//
//  slave.c
//  esb
//
//  Created by Evan Andersen on 12-12-14.
//  Copyright (c) 2012 Evan Andersen. All rights reserved.
//

#include "slave.h"

json_t* readCommand(int fd, int timeout)
{
    struct timeval start;
    gettimeofday(&start, NULL);
    
    uint32_t expectedSize = 0;
    
    char buf[4096];

    while(1)
    {
        char sizeBuf[9];
        ssize_t result = recv(fd, sizeBuf, sizeof(sizeBuf) - 1, MSG_DONTWAIT);
        if(result == -1 && !(errno == EAGAIN || errno == EWOULDBLOCK))
        {
            perror("recv failed");
            return NULL;
        }
        if(result == 0)
        {
            fprintf(stderr, "Remote hung up unexpectedly\n");
            return NULL;
        }
        if(result == (sizeof(sizeBuf) - 1))
        {
            sizeBuf[8] = '\0';
            expectedSize = atoi(sizeBuf);
            if(expectedSize > (sizeof(buf) -1) || expectedSize == 0)
            {
                fprintf(stderr, "Packet with incorrect size recived (%d)\n", expectedSize);
                return NULL;
            }
            break;
        }
        
        if(timeElapsed(&start) < timeout * 10)
        {
            usleep(1000);
            continue;
        }
        return NULL;
    }
    
    
    uint32_t amountRead = 0;
    while(amountRead < expectedSize)
    {
        ssize_t result = recv(fd, buf + amountRead, expectedSize - amountRead, MSG_DONTWAIT);
        if(result == -1 && !(errno == EAGAIN || errno == EWOULDBLOCK))
        {
            perror("recv failed");
            return NULL;
        }
        if(result == 0)
        {
            fprintf(stderr, "Remote hung up unexpectedly\n");
            return NULL;
        }
        if(result > 0)
        {
            amountRead += result;
        }
        else
        {
            if(timeElapsed(&start) < timeout * 10)
            {
                usleep(10000);
                continue;
            }
            return NULL;
        }
    }
    buf[expectedSize] = '\0';
    json_error_t err;
    json_t* root = json_loads(buf, 0, &err);
    if(!root)
    {
        fprintf(stderr, "Error parsing JSON on Line %d: %s\n", err.line, err.text);
    }
    return root;
}

//split a group of a few chunks into many
void splitTasks(struct workerTask* single, struct workerTask* out, int count, void** connPool)
{
    uint64_t targetCount = single->count/count;
    uint64_t targetExtra = single->count % count;
    
    uint64_t target = single->numKeys/count;
    uint64_t extra = single->numKeys % count;
    uint64_t currentKey = single->startingKey;

    double targetThroughput = single->throughput/(double)count;
    
    uint32_t newConnections = 0;
    
    for(int i = 0; i < count; i++)
    {
        //copy the other information
        memcpy(&out[i], single, sizeof(struct workerTask));
        out[i].throughput = targetThroughput;

        out[i].workerID = single->workerID + i;
        
        
        out[i].conn = connPool[i];
        if(!out[i].conn)
        {
            newConnections++;
        }
        out[i].connOpenDelay = newConnections/kNewConnectionsPerMs;
        
        out[i].count = targetCount + (i < targetExtra);
        
        uint64_t keysToAdd = target + (i < extra);
        if(single->values)
        {
            out[i].values = &single->values[currentKey];
        }
        else
        {
            out[i].values = NULL;
        }
        out[i].numKeys = keysToAdd;
        out[i].startingKey = currentKey;
        
        currentKey += keysToAdd;
    }
}

void performRequest(struct workerTask* tasks, int taskCount, json_t** result)
{
    uint64_t* latencyResults = __xmalloc(sizeof(uint64_t) * kNumLatencyBuckets);
    memset(latencyResults, 0, sizeof(uint64_t) * kNumLatencyBuckets);
    
    //create a thread for each client
    pthread_t* threads = __xmalloc(sizeof(pthread_t) * taskCount);
    for(int i = 0; i < taskCount; i++)
    {
        //don't start a worker if it does'nt have any keys
        if(tasks[i].numKeys == 0)
        {
            taskCount = i;
            break;
        }
        
        tasks[i].latencyResults = latencyResults;
        
        //TODO - possibly reduce stack size for clients
        pthread_attr_t attributes;
        pthread_attr_init(&attributes);
        int result = pthread_attr_setstacksize(&attributes, 512*1024);
        if(result)
        {
            char buf[256];
            strerror_r(result, buf, sizeof(buf));
            fprintf(stderr, "Error setting stack size of thread: %s\n", buf);
            taskCount = i;
            break;
        }
        
        result = pthread_create(&threads[i], &attributes, testClient, &tasks[i]);
        if(result)
        {
            char buf[256];
            strerror_r(result, buf, sizeof(buf));
            fprintf(stderr, "Error creating thread %d: %s\n", i, buf);
            taskCount = i;
            break;
        }
    }
    
    //get the results
    
    //TODO add a timeout to allow the forceful killing of threads
    
    
    //create a json object to store the results
    *result = json_object();
    json_t* errorArray = json_array();
    json_t* resultArray = json_array();
    json_object_set_new(*result, "errors", errorArray);
    json_object_set_new(*result, "latency", resultArray);
    
    for(int i = 0; i < taskCount; i++)
    {
        void* threadResult;
        pthread_join(threads[i], &threadResult);
        
        //returns a pointer to an error message - NULL means no error
        if(threadResult)
        {
            json_t* newError = json_object();
            json_object_set_new(newError, "cause", json_string(threadResult));
            json_object_set_new(newError, "client-id", json_integer(tasks[i].workerID));
            json_array_append_new(errorArray, newError);
        }
    }
    
    //add the latency stats to the json object
    for(int i = 0; i < kNumLatencyBuckets; i++)
    {
        if(latencyResults[i])
        {
            json_array_append_new(resultArray, json_integer(i));
            json_array_append_new(resultArray, json_integer(latencyResults[i]));
        }
    }
    free(threads);
    free(latencyResults);
}

int compareTimes(const struct timeval* a, const struct timeval* b)
{
    if(a->tv_sec == b->tv_sec)
    {
        return a->tv_usec > b->tv_usec;
    }
    return a->tv_sec > b->tv_sec;
}


int beginSlavery(int fd)
{
    uint16_t randContext[3];
    for(int i = 0; i < 3; i++)
    {
        randContext[i] = time(NULL) ^ getpid();
    }
    
    int returnCode = -1;
    
    json_t* params = NULL;
    json_t* nextCommand = NULL;
    
    char** values = NULL;
    
    struct workerTask* tasks = NULL;
    
    uint32_t connPoolSize = 128;
    uint32_t connPoolCount = 0;
    void** connPool = __xmalloc(sizeof(void*) * connPoolSize);
    memset(connPool, 0, sizeof(void*) * connPoolSize);
    
    //recieve initial parameters
    params = readCommand(fd, 5000);
    if(!params)
    {
        fprintf(stderr, "No initialization packet\n");
        goto exit;
    }
    
    //make sure the initial packet has all of the required info
    uint64_t slaveID;
    const char* command;
    const char* address;
    const char* username;
    const char* password;
    const char* tableName;
    int port;
    int errorChecking;
    int valueLength;
    int result = json_unpack(params, "{s:s, s:s, s:i, s:I, s:s, s:s, s:s, s:b, s:i}",
                             "command",         &command,
                             "address",         &address,
                             "port",            &port,
                             "slave-id",        &slaveID,
                             "username",        &username,
                             "password",        &password,
                             "table",           &tableName,
                             "error-checking",  &errorChecking,
                             "value-length",    &valueLength);
    if(result != 0 || strcmp(command, "init") || port <= 0 || port > 65535)
    {
        fprintf(stderr, "Invalid initialization packet recieved\n");
        goto exit;
    }

    uint64_t numKeys = 0;
    
    //loop and execute commands from the master
    while(1)
    {
        nextCommand = readCommand(fd, 15000);
        if(!nextCommand)
        {
            fprintf(stderr, "Invalid next command\n");
            goto exit;
        }
                        
        const char* command;
        int result = json_unpack(nextCommand, "{s:s}", "command", &command);
        if(result == -1)
        {
            fprintf(stderr, "Packet missing command field\n");
            goto exit;
        }
        
        //parse commands and modify keyRange array if needed
        struct workerTask task;
        
        if(!strcmp(command, "add"))
        {
            uint64_t numKeysToAdd;
            int result = json_unpack(nextCommand, "{s:I}", "amount", &numKeysToAdd);
            if(result == -1)
            {
                goto exit;
            }

            if(errorChecking)
            {
                values = __xrealloc(values, sizeof(char*) * (numKeys + numKeysToAdd));
                for(uint64_t i = 0; i < numKeysToAdd; i++)
                {
                    values[numKeys + i] = __xmalloc(valueLength + 1);
                    for(int j = 0; j < valueLength; j++)
                    {
                        values[numKeys + i][j] = (erand48(randContext) * 26) + 'A';
                    }
                    values[numKeys + i][valueLength] = '\0';
                }
            }
            task.startingKey = slaveID + numKeys;
            task.numKeys = numKeysToAdd;
            task.type = kClientAddKeys;
            task.values = values;
            numKeys += task.numKeys;
        }
        else if(!strcmp(command, "remove"))
        {
            uint64_t numKeysToRemove;
            
            int result = json_unpack(nextCommand, "{s:I}", "amount", &numKeysToRemove);
            if(result == -1)
            {
                goto exit;
            }
            
            if(errorChecking)
            {
                uint64_t position = numKeys - numKeysToRemove;
                for(uint64_t i = 0; i < numKeysToRemove; i++)
                {
                    free(values[position + i]);
                }
                values = __xrealloc(&values, sizeof(char*) * (numKeys - numKeysToRemove));
            }
            if(numKeysToRemove > numKeys)
            {
                goto exit;
            }
            task.startingKey = slaveID + numKeys - numKeysToRemove;
            task.numKeys = numKeysToRemove;
            task.type = kClientRemoveKeys;
            task.values = NULL;
            numKeys -= numKeysToRemove;
        }
        else if(!strcmp(command, "test"))
        {
            json_t* array = json_object_get(nextCommand, "workload");

            //right now we only need 1 workload type
            if(!(json_is_array(array) && json_array_size(array) == kWorkloadTypes))
            {
                goto exit;
            }
            for(int i = 0; i < kWorkloadTypes; i++)
            {
                task.workloadComposition[i] = json_number_value(json_array_get(array, i));
            }
            int result = json_unpack(nextCommand, "{s:I}", "amount", &task.count);
            if(result == -1)
            {
                goto exit;
            }
            task.startingKey = slaveID;
            task.numKeys = numKeys;
            task.values = values;
            task.type = kClientRunWorkload;
        }
        else if(!strcmp(command, "quit"))
        {
            returnCode = 0;
            goto exit;
        }
        else
        {
            fprintf(stderr, "unknown command from client: %s\n", command);
            goto exit;
        }
        
        int numClients;
        result = json_unpack(nextCommand, "{s:i, s:F}", "num-clients", &numClients, "throughput", &task.throughput);
        if(result == -1)
        {
            fprintf(stderr, "Packet missing number of clients or throughput value\n");
            goto exit;
        }
                
        task.connOpenDelay = 0;
        
        //fill out the generic task information
        task.table = tableName;
        task.valueSize = valueLength;
        task.workerID = slaveID;
        task.hostname = address;
        task.username = username;
        task.password = password;
        task.port = port;
        
        //split the request between the clients
        while(numClients >= connPoolSize)
        {
            size_t oldSize = connPoolSize;
            connPoolSize *= 8;
            connPool = __xrealloc(connPool, connPoolSize * sizeof(void*));
            memset(&connPool[oldSize], 0, sizeof(void*) * oldSize * 7);
        }
        
        if(numClients < connPoolCount)
        {
            for(int i = connPoolCount - 1; i >= numClients; i--)
            {
                storage_disconnect(connPool[i]);
                connPool[i] = NULL;
            }
        }
        
        tasks = __xmalloc(sizeof(struct workerTask) * numClients);
        splitTasks(&task, tasks, numClients, connPool);
                
        //perform the request
        json_t* requestResponse;
        performRequest(tasks, numClients, &requestResponse);
        
        //save the connections for next time
        for(int i = 0; i < numClients; i++)
        {
            connPool[i] = tasks[i].conn;
        }
        connPoolCount = numClients;
        
        //send the result back to the master
        char* serialResponse = json_dumps(requestResponse, JSON_COMPACT);
        json_decref(requestResponse);
        size_t len = strlen(serialResponse);
        if(len > 1048576)
        {
            fprintf(stderr, "Response too large (%zd)\n", len);
            free(serialResponse);
            goto exit;
        }
        char sizeBuf[9];
        sprintf(sizeBuf, "%08d", (int)len);
        sendAll(fd, sizeBuf, 8);
        
        if(sendAll(fd, serialResponse, strlen(serialResponse)) == -1)
        {
            free(serialResponse);
            goto exit;
        }
        free(serialResponse);

        free(tasks);
        tasks = NULL;        
        
        //free the parsed command
        json_decref(nextCommand);
        nextCommand = NULL;
    }
    
exit:
    for(uint32_t i = 0; i < connPoolCount; i++)
    {
        storage_disconnect(connPool[i]);
    }
    free(connPool);
    free(tasks);
    json_decref(nextCommand);
    json_decref(params);
    if(values)
    {
        for(uint64_t i = 0; i < numKeys; i++)
        {
            free(values[i]);
        }
        free(values);
    }
    return returnCode;
}
