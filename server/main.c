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
#include <argtable2.h>
#include <stdint.h>
#include <poll.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>

#include "global.h"
#include "network.h"
#include "slave.h"

int verbose;

#define PROGRAM_VERSION "0.0.1"

void ignore(int signal)
{
    
}

int main(int argc, char** argv)
{
    srand((int)time(NULL));
    
    struct arg_lit *help = arg_lit0("h", "help", "print this help");
    struct arg_int *port = arg_int0("p", "port","<n>", "Port used for communicating with master");
    struct arg_lit *verb = arg_lit0("v", "verbose", "verbose output");
    struct arg_end *end = arg_end(20);
    void *argtable[] = {help, port, verb, end};
    
    int errors = arg_parse(argc, argv, argtable);
    if(help->count == 1)
    {
        printf("\nECE297 Server Benchmark v"PROGRAM_VERSION" (Client)\n\nUsage:\n");
        arg_print_glossary(stdout, argtable, "  %-30s %s\n");
        printf("\n");
        return 0;
    }
    
    //increase open file count
    struct rlimit limit;
    if(getrlimit(RLIMIT_NOFILE, &limit))
    {
        perror("Failed to get limit");
        return -1;
    }
    
    //OSX maximum
    limit.rlim_cur = 10240;
    
    if(setrlimit(RLIMIT_NOFILE, &limit))
    {
        perror("Failed to set limit");
        return -1;
    }    
        
    if(errors > 0)
    {
        arg_print_errors(stderr, end, "esb");
        return -1;
    }
    
    verbose = verb->count;
    
    int portToUse = 4912;
    if(port->count == 1)
    {
        portToUse = *port->ival;
    }
    
    arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));

    
    int listenFD = createListenFD(portToUse);
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
        struct sockaddr_in6 peerAddr;
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
        else if(verbose)
        {
            char buf[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, &peerAddr, buf, len);
            printf("Recieved connection from %s\n", buf);
        }
        
        //handle the slavery request (abe wouldn't like this)
        beginSlavery(newFD);
        close(newFD);
    }
    
    return 0;
}
