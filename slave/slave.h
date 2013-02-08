//
//  slave.h
//  esb
//
//  Created by Evan Andersen on 12-12-14.
//  Copyright (c) 2012 Evan Andersen. All rights reserved.
//

#ifndef esb_slave_h
#define esb_slave_h

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include "jansson.h"

#include "network.h"
#include "tester.h"

#define kNewConnectionsPerMs 8

int beginSlavery(int fd);


#endif
