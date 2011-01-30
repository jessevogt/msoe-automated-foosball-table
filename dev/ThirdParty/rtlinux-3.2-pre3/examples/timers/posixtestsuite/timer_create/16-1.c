/*   
 * Copyright (c) 2002, Intel Corporation. All rights reserved.
 * Created by:  julie.n.fleischer REMOVE-THIS AT intel DOT com
 * This file is licensed under the GPL license.  For the full content
 * of this license, see the COPYING file at the top level of this 
 * source tree.

 * Test that timer_create() sets errno to EINVAL if clock_id is not
 * a defined clock ID.
 */

#include <time.h>
#include <signal.h>
#include "compat.h"
#include <errno.h>

#define PASS 0
#define FAIL 1
#define UNRESOLVED 2

#define INVALIDCLOCKID 99999

int init_module(void)
{
	struct sigevent ev;
	timer_t tid;

	ev.sigev_notify = SIGEV_SIGNAL;
	ev.sigev_signo = SIGALRM;

	if (timer_create(INVALIDCLOCKID, &ev, &tid) == -1) {
		if (EINVAL == errno) {
			printf("Test PASSED\n");
			return PASS;
		} else {
			printf("errno != EINVAL\n");
			printf("Test FAILED\n");
			return FAIL;
		}
	} else {
		printf("timer_create returned success\n");
		return UNRESOLVED;
	}
	return UNRESOLVED;
}

void cleanup_module(void){}
