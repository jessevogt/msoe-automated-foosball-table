/*   
 * Copyright (c) 2002, Intel Corporation. All rights reserved.
 * Created by:  julie.n.fleischer REMOVE-THIS AT intel DOT com
 * This file is licensed under the GPL license.  For the full content
 * of this license, see the COPYING file at the top level of this 
 * source tree.

 * Test that the following scenarios are identical:
 * evp == NULL
 * and
 * evp.sigev_notify=SIGEV_SIGNAL
 * evp.sigev_signo="default signal"
 * evp.sigev_value=timerid
 *
 * Steps:
 * 1.  Set up sigaction structure to catch signal SIGALRM (default signal) on 
 *     timer expiration
 * 2.  Create timer using timer_create().
 * 3.  Activate timer using timer_settime() and then sleep.
 * 4.  If signal handler is called, continue.  Otherwise, fail.
 * 5.  If signal handler was called and the time left in sleep ~= the
 *     delta between the timer time and sleep time, then PASS.
 *     Otherwise, FAIL.
 *
 * Note:  This test is identical to 1-1.c minus the evp lines.
 *
 * For this test clock CLOCK_REALTIME will be used.
 */

#include <time.h>
#include <signal.h>
#include "compat.h"

#define PASS 0
#define FAIL 1
#define UNRESOLVED 2

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
		perror("In RTLinux SIGALARM doesn't exists!\n");
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

int init_module(void){
  int err=0;

	if (timer_create(CLOCK_REALTIME, NULL, &tid) != 0) {
		perror("timer_create() did not return success\n");
		return UNRESOLVED;
	}

	err=pthread_create(&th,NULL,th_code,0);

	return err;

}

void cleanup_module(void){
  timer_delete(tid);
  pthread_delete_np(th);
}





















