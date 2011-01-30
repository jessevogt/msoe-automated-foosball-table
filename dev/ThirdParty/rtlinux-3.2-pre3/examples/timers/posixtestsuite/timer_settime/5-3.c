/*   
 * Copyright (c) 2002, Intel Corporation. All rights reserved.
 * Created by:  julie.n.fleischer REMOVE-THIS AT intel DOT com
 * This file is licensed under the GPL license.  For the full content
 * of this license, see the COPYING file at the top level of this 
 * source tree.

 * Test that if timer_settime() is using an absolute clock and the
 * time has already taken place when the test is running that
 * timer_settime() succeeds and the expiration notification is made.
 *
 * Test for a variety of times which keep getting SUBTRACTAMOUNT
 * of time away from the time at the start of the test.
 * Note:  This test was made in response to a bug seen where timers
 *        would return -1 intermittently on time values generally
 *        > 9000 seconds before the current time.
 */

#include <time.h>
#include <signal.h>
#include "compat.h"

#define PASS 0
#define FAIL 1
#define UNRESOLVED 2

#define SIGTOTEST SIGALRM

#define LONGSLEEPTIME 10

#define NUMTESTS 30

#define SUBTRACTAMOUNT 1000

int fails = 0, passes = 0;
timer_t tid;
pthread_t th;

void handler(int signo)
{
	printf("Caught signal\n");
	passes += 1;
}

void *th_code(void *arg)
{
	struct sigaction act;
	struct itimerspec its;
	struct timespec ts;
	int flags = 0;
	int i;

	act.sa_handler=handler;
	act.sa_flags=0;

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

	if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
		perror("clock_gettime() did not return success\n");
		return UNRESOLVED;
	}

	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = 0;
	its.it_value.tv_sec = ts.tv_sec;
	its.it_value.tv_nsec = 0;

	flags |= TIMER_ABSTIME;
	for (i = 0; i < NUMTESTS; i++) {
		if (timer_settime(tid, flags, &its, NULL) != 0) {
			printf("failure at %d\n",
					(int) its.it_value.tv_sec);
		} else {
			sleep(LONGSLEEPTIME);
		}
		its.it_value.tv_sec -= SUBTRACTAMOUNT;
	}
	fails = NUMTESTS-passes;

	printf("passes %d, fails %d\n", passes, fails);

	if (fails > 0) {
		return FAIL;
	} else {
		return PASS;
	}

	return UNRESOLVED;
}

int init_module(void){
  struct sigevent ev;
  int err=0;
  
  ev.sigev_notify = SIGEV_SIGNAL;
  ev.sigev_signo = SIGTOTEST;

  if (timer_create(CLOCK_REALTIME, &ev, &tid) != 0) {
    perror("timer_create() did not return success\n");
    return UNRESOLVED;
  }
  
  err=pthread_create(&th,NULL,th_code,(void *) 0);

  return err;
  
}

void cleanup_module(void){
  timer_delete(tid);
  pthread_delete_np(th);
}




































