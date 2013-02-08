//
//  network.h
//  esb_client
//
//  Created by Evan Andersen on 12-12-20.
//  Copyright (c) 2012 Evan Andersen. All rights reserved.
//

#ifndef esb_client_network_h
#define esb_client_network_h

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "jansson.h"

#include "global.h"

#define kMaxPacketSize 32768

#define kSendBufSize 16

struct stream
{
    int fd;
    uint32_t recvPos;
    uint64_t sendPos;
    uint64_t sendQueuePos;
    char lastRecvPacket[kMaxPacketSize];
    char recvBuffer[kMaxPacketSize];
    struct iovec writeBuffer[kSendBufSize];
};


int createListenFD(int port);
int sendAll(int socket, const char* buf, size_t num);





#endif
