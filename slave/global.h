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
#include "md5.h"

extern int verbose;

void* xmalloc(size_t size);
void* xrealloc(void* pointer, size_t size);


//a random number on the range [0, 1)
double randomZeroToOne();

uint64_t timeElapsed(struct timeval* start);
int64_t uSElapsed(struct timeval* start);
void hashInt64(uint64_t input, char* output);


#endif
