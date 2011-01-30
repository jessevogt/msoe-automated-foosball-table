/*
 * (C) Finite State Machine Labs Inc. 2000 business@fsmlabs.com
 *
 * Released under the terms of GPL 2.
 * Open RTLinux makes use of a patented process described in
 * US Patent 5,995,745. Use of this process is governed
 * by the Open RTLinux Patent License which can be obtained from
 * www.fsmlabs.com/PATENT or by sending email to
 * licensequestions@fsmlabs.com
 */

#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

struct timeval torig, t1, t2;

int main(void)
{
#define US(x) ( ((unsigned long long)(x).tv_sec*1000000) + (unsigned long long)(x).tv_usec )
	gettimeofday(&torig, 0);
	t1 = torig;
	do {
		gettimeofday(&t2, 0);
		if (US(t2) < US(t1)) {
			fprintf(stderr, "Linux time went backwards\n");
			return -1;
		}
		t1 = t2;
	} while (US(t2) < (US(torig) + (unsigned long long) 10 * 1000000));
	return 0;
}
