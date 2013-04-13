//
//  global.h
//  esb
//
//  Created by Evan Andersen on 12-12-14.
//  Copyright (c) 2012 Evan Andersen. All rights reserved.
//

#ifndef esb_global_h
#define esb_global_h

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>
#include "storage_protocol.h"
#include "md5.h"


void* __xmalloc(size_t size);
void* __xrealloc(void* pointer, size_t size);

uint64_t timeElapsed(struct timeval* start);
int64_t uSElapsed(struct timeval* start);
void hashInt64(uint64_t input, char* output);
const char* ece297strerror(int error);


#endif
