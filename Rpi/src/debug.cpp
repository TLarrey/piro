/*
 * debug.cpp
 *
 *  Created on: 22-Oct-2013
 *      Author: TLarrey
 */
#include "debug.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <math.h>


int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y)
{
  /* Perform the carry for the later subtraction by updating y. */
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}

struct timeval initTime;

char * GetTimeStamp() {
    struct timeval current_tv;
    struct timeval result_tv;
    char tab[20]="";
    gettimeofday(&current_tv,0);
    if(!timeval_subtract(&result_tv,&current_tv,&initTime)){
    	sprintf(tab,"%03ld:%06ld",result_tv.tv_sec,result_tv.tv_usec);
    }
    else {
    	sprintf(tab,"xxx:xxxxxx");
    }
    return tab;
}



