//
//  main.c
//  esb_client
//
//  Created by Evan Andersen on 12-12-14.
//  Copyright (c) 2012 Evan Andersen. All rights reserved.
//

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <poll.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/time.h>

#include "global.h"
#include "network.h"
#include "slave.h"

void ignore(int signal)
{
    
}

int main(int argc, char** argv)
{
    srand((int)time(NULL));

    
    //increase open file count
    struct rlimit limit;
    if(getrlimit(RLIMIT_NOFILE, &limit))
    {
        perror("Failed to get limit");
        return -1;
    }
    
    //OSX maximum
#ifdef __APPLE__
    limit.rlim_cur = 10240;
#else
    limit.rlim_cur = limit.rlim_max;
#endif
    
    if(setrlimit(RLIMIT_NOFILE, &limit))
    {
        perror("Failed to set limit");
        return -1;
    }    
        
    int listenFD = createListenFD(4912);
    if(listenFD == -1)
    {
        return -1;
    }
    
    //ignore sigpipe, it's stupid anyways
    struct sigaction new_sa;
    struct sigaction old_sa;
    sigfillset(&new_sa.sa_mask);
    new_sa.sa_handler = SIG_IGN;
    new_sa.sa_flags = 0;
    
    if(sigaction(SIGPIPE, &new_sa, &old_sa) == 0 && old_sa.sa_handler != SIG_IGN)
    {
        new_sa.sa_handler = ignore;
        sigaction(SIGPIPE, &new_sa, 0);
    }
    
    while(1)
    {
        struct sockaddr_in peerAddr;
        socklen_t len = sizeof(peerAddr);
        int newFD = accept(listenFD, (struct sockaddr*)&peerAddr, &len);
        if(newFD == -1)
        {
            if(errno != EWOULDBLOCK && errno != EAGAIN && errno != EINTR)
            {
                perror("Accept failed");
            }
            continue;
        }
        char buf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &peerAddr, buf, len);
        printf("Recieved connection from %s\n", buf);
        
        //handle the slavery request (abe wouldn't like this)
        beginSlavery(newFD);
        close(newFD);
    }
    
    return 0;
}
