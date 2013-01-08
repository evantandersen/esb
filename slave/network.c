//
//  network.c
//  esb_client
//
//  Created by Evan Andersen on 12-12-20.
//  Copyright (c) 2012 Evan Andersen. All rights reserved.
//

#include "network.h"

int getIP(char* buffer, size_t buflen)
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock == -1)
    {
        perror("Socket failed");
        return -1;
    }
    
    const char* kGoogleDnsIp = "8.8.8.8";
    uint16_t kDnsPort = 53;
    struct sockaddr_in serv;
    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = inet_addr(kGoogleDnsIp);
    serv.sin_port = htons(kDnsPort);
    
    int err = connect(sock, (const struct sockaddr*) &serv, sizeof(serv));
    if(err == -1)
    {
        perror("Connect failed");
        return -1;
    }
    
    struct sockaddr_in name;
    socklen_t namelen = sizeof(name);
    err = getsockname(sock, (struct sockaddr*) &name, &namelen);
    
    if(!inet_ntop(AF_INET, &name.sin_addr, buffer, (int)buflen))
    {
        fprintf(stderr, "inet_ntop failed\n");
        return -1;
    }
    
    close(sock);
    return 0;
}


int createListenFD(int port)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    
    struct addrinfo* serverList;
    char buf[64];
    sprintf(buf, "%d", port);
    
    char addrBuf[128];
    if(getIP(addrBuf, sizeof(addrBuf)))
    {
        return -1;
    }
    
    int result = getaddrinfo(addrBuf, buf, &hints, &serverList);
    if(result)
    {
        fprintf(stderr, "Error getting address info");
        return -1;
    }
    
    //two sockets - IPv4 and IPv6
	char addressString[INET6_ADDRSTRLEN];
	
    struct addrinfo* try;
    int socketFD = -1;
    for(try = serverList; try; try = try->ai_next)
    {
        if(try->ai_family == AF_INET)
        {
            socketFD = socket(try->ai_family, try->ai_socktype, try->ai_protocol);
            if(socketFD == -1)
            {
                perror("Error opening socket");
                continue;
            }
            
            if(try->ai_family == AF_INET)
            {
                inet_ntop(try->ai_family, &((struct sockaddr_in*)try->ai_addr)->sin_addr, addressString, sizeof(addressString));
            }
            else
            {
                inet_ntop(try->ai_family, &((struct sockaddr_in6*)try->ai_addr)->sin6_addr, addressString, sizeof(addressString));
            }
            
            int yes = 1;
            if(setsockopt(socketFD, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
            {
                perror("Error setting socket options");
                return -1;
            }
            
            if(bind(socketFD, try->ai_addr, try->ai_addrlen) == -1)
            {
                perror("Binding Server Failed");
                return -1;
            }
            break;
        }
    }
    freeaddrinfo(serverList);
    
    if(socketFD == -1)
    {
        fprintf(stderr, "Failed to create any listening sockets\n");
        return -1;
    }
    
    //listen with a queue of 10
    if(listen(socketFD, 20) == -1)
    {
        perror("Listen failed");
        return -1;
    }
    else
    {
        fprintf(stdout, "Listening on: %s\n", addressString);
    }
    
    return socketFD;
}


int sendAll(int socket, const char* buf, size_t num)
{
    while(num)
    {
        ssize_t result = send(socket, buf, num, 0);
        if(result == -1)
        {
            perror("Failed to send data");
            return -1;
        }
        num -= result;
    }
    return 0;
}

