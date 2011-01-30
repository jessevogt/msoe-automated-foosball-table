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

#include <rtlinux_debug.h>
#include <rtlinux_signal.h>
#include <stdio.h>
#include <errno.h>

int count;
volatile int *countP;

void div0_generator(int whatever)
{
	/* attempt to divide by zero - no, this doesn't return infinity */
	*countP = 1 / 0;
}

int main(void)
{
	struct rtlinux_sigaction mysig, oldsig;
	int i = 5;
	countP = &count;

	mysig.sa_handler = div0_generator;
	mysig.sa_flags = RTLINUX_SA_ONESHOT;
	mysig.sa_period = NSECS_PER_SEC / 4;

	if (rtlinux_sigaction(RTLINUX_SIGTIMER0, &mysig, &oldsig)) {
		perror("rtlinux_sigaction");
		return errno;
	}

	fprintf(stderr, "Restoring signal handler to default in %d\n", i);
	while (i > 0) {
		sleep(1);
		i--;
		fprintf(stderr, "%d\n", i);
	}

	mysig.sa_handler = RTLINUX_SIG_DFL;
	if (rtlinux_sigaction(RTLINUX_SIGTIMER0, &mysig, &oldsig)) {
		perror("rtlinux_sigaction");
		return errno;
	}

	return 0;
}
