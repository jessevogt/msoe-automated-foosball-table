/*   
 * Copyright (c) 2002, Intel Corporation. All rights reserved.
 * Created by:  julie.n.fleischer REMOVE-THIS AT intel DOT com
 * This file is licensed under the GPL license.  For the full content
 * of this license, see the COPYING file at the top level of this 
 * source tree.

 * pt:MON
 *
 * Test that CLOCK_MONOTONIC is supported by timer_create().
 *
 * Same test as 1-1.c with CLOCK_MONOTONIC.
 */

#include <time.h>
#include <signal.h>
#include "compat.h"


#define PASS 0
#define FAIL 1
#define UNRESOLVED 2
#define UNSUPPORTED 4

#define SIGTOTEST SIGALRM
#define TIMERSEC 2
#define SLEEPDELTA 3
#define ACCEPTABLEDELTA 1

void handler(int signo)
{
	printf("Caught signal\n");
}

timer_t tid;
pthread_t th;


void *th_code(void *arg)
{
	struct sigaction act;
	struct itimerspec its;
	struct timespec ts, tsleft;

	act.sa_handler=handler;
	act.sa_flags=0;

	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = 0;
	its.it_value.tv_sec = TIMERSEC;
	its.it_value.tv_nsec = 0;

	ts.tv_sec=TIMERSEC+SLEEPDELTA;
	ts.tv_nsec=0;

	/*
	if (sigemptyset(&act.sa_mask) == -1) {
		perror("Error calling sigemptyset\n");
		return UNRESOLVED;
	}
	*/
	if (sigaction(SIGTOTEST, &act, 0) == -1) {
		perror("Error calling sigaction\n");
		return UNRESOLVED;
	}


	if (timer_settime(tid, 0, &its, NULL) != 0) {
		perror("timer_settime() did not return success\n");
		return UNRESOLVED;
	}

	if (nanosleep(&ts, &tsleft) != -1) {
		perror("nanosleep() not interrupted\n");
		return FAIL;
	}

	if ( abs(tsleft.tv_sec-SLEEPDELTA) <= ACCEPTABLEDELTA) {
		printf("Test PASSED\n");
		return PASS;
	} else {
		printf("Timer did not last for correct amount of time\n");
		printf("timer: %d != correct %d\n", 
				(int) ts.tv_sec- (int) tsleft.tv_sec,
				TIMERSEC);
		return FAIL;
	}

	return FAIL;

}


int init_module(void)
{
  int err=0;
  struct sigevent ev;
  ev.sigev_notify = SIGEV_SIGNAL;
  ev.sigev_signo = SIGTOTEST;

#ifdef CLOCK_MONOTONIC

	if (timer_create(CLOCK_MONOTONIC, &ev, &tid) != 0) {
		perror("timer_create() did not return success\n");
		return UNRESOLVED;
	}

#else
	printf("CLOCK_MONOTONIC unsupported\n");
	return UNSUPPORTED;
#endif



  err=pthread_create(&th,NULL,th_code,0);

  return err;

}

void cleanup_module(void){
#ifdef CLOCK_MONOTONIC
  timer_delete(tid);
  pthread_delete_np(th);
#endif
}


