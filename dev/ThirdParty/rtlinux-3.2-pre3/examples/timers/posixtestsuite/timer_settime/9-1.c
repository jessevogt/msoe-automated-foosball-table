/*   
 * Copyright (c) 2002, Intel Corporation. All rights reserved.
 * Created by:  julie.n.fleischer REMOVE-THIS AT intel DOT com
 * This file is licensed under the GPL license.  For the full content
 * of this license, see the COPYING file at the top level of this 
 * source tree.
 *
 * Test that timers are not allowed to expire before their scheduled
 * time.
 *
 * Test for a variety of timer values on relative timers.
 *
 * For this test, signal SIGTOTEST will be used, clock CLOCK_REALTIME
 * will be used.
 */

#include <time.h>
#include <signal.h>
#include "compat.h"

#define PASS 0
#define FAIL 1
#define UNRESOLVED 2

#define SIGTOTEST SIGALRM
#define TIMERVALUESEC 2
#define TIMERINTERVALSEC 5
#define INCREMENT 1
#define ACCEPTABLEDELTA 1

#define NUMTESTS 6

static int timeroffsets[NUMTESTS][2] = { {0, 30000000}, {1, 0}, 
					{1, 30000000}, {2, 0},
					{1, 5000}, {1, 5} };
timer_t tid;
pthread_t th;

void handler(int sig){
  printf("Signal caught\n");
}

#define sigwait(set,sig) (int)({int mask=~(*set); sigsuspend(&mask); 0; } )

void *th_code(void *arg)
{
  struct itimerspec its;
	struct timespec tsbefore, tsafter;
	struct sigaction act;
	rtl_sigset_t set;
	int sig;
	int i;
	int failure = 0;
	unsigned long totalnsecs, testnsecs; // so long was we are < 2.1 seconds, we should be safe

	/*
	 * set up signal set containing SIGTOTEST that will be used
	 * in call to sigwait immediately after timer is set
	 */

	rtl_sigemptyset(&set);
	rtl_sigaddset(&set, SIGTOTEST);
	
	act.sa_handler=handler;
	
	if (sigaction(SIGTOTEST, &act, 0) == -1) {
	  perror("Error calling sigaction\n");
	  return UNRESOLVED;
	}
	
	for (i = 0; i < NUMTESTS; i++) {
		its.it_interval.tv_sec = 0; its.it_interval.tv_nsec = 0;
		its.it_value.tv_sec = timeroffsets[i][0];
		its.it_value.tv_nsec = timeroffsets[i][1];

		printf("Test for value %d sec %d nsec\n", 
				(int) its.it_value.tv_sec,
				(int) its.it_value.tv_nsec);

		if (clock_gettime(CLOCK_REALTIME, &tsbefore) != 0) {
			perror("clock_gettime() did not return success\n");
			return UNRESOLVED;
		}
		
		if (timer_settime(tid, 0, &its, NULL) != 0) {
			perror("timer_settime() did not return success\n");
			return UNRESOLVED;
		}
	
		if (sigwait(&set, &sig) == -1) {
			perror("sigwait() failed\n");
			return UNRESOLVED;
		}
	
		if (clock_gettime(CLOCK_REALTIME, &tsafter) != 0) {
			perror("clock_gettime() did not return success\n");
			return UNRESOLVED;
		}
	
		totalnsecs = (unsigned long) (tsafter.tv_sec-tsbefore.tv_sec)*
					1000000000 +
					(tsafter.tv_nsec-tsbefore.tv_nsec);
		testnsecs = (unsigned long) its.it_value.tv_sec*1000000000 + 
					its.it_value.tv_nsec;
		printf("total %lu test %lu\n", totalnsecs, testnsecs);
		if (totalnsecs < testnsecs) {
			printf("FAIL:  Expired %ld < %ld\n", totalnsecs,
							testnsecs);
			failure = 1;
		}
	}

	if (failure) {
		printf("timer_settime() failed on at least one value\n");
		return FAIL;
	} else {
		printf("Test PASSED\n");
		return PASS;
	}
}

int init_module(void){
  struct sigevent ev;
  int err=0;
  
  /*
   * set up timer to perform action SIGTOTEST on expiration
   */
  ev.sigev_notify = SIGEV_SIGNAL;
  ev.sigev_signo = SIGTOTEST;
  
  if (timer_create(CLOCK_REALTIME, &ev, &tid) != 0) {
    perror("timer_create() did not return success\n");
    return UNRESOLVED;
  }
  
  err=pthread_create(&th,NULL,th_code,(void *) 0);

  return err;
  
}

void cleanup_module(void)
{
  if (timer_delete(tid) != 0) {
    perror("timer_delete() did not return success\n");
    return UNRESOLVED;
  }
  
  pthread_delete_np(th);
}
