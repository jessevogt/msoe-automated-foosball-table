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
#include <rtlinux_signal.h>

unsigned long marker = 0, marker2 = 0;
volatile unsigned long *marker_ptr = &marker, *marker2_ptr = &marker2;

void handler(int unused)
{
	*marker_ptr += 1;
}

void handler2(int unused)
{
	*marker2_ptr += 1;
}

int main(void)
{
	struct rtlinux_sigaction sig, sig2, oldsig;
	rtlinux_sigset_t myset, oldset;

	fprintf(stderr, "Testing oneshot timer\n");
	/* setup for a test of timer signal masks */
	if ((rtlinux_sigemptyset(&myset)) != 0) {
		perror("rtlinux_sigemptyset(&myset)");
		return (-1);
	}

	if ((rtlinux_sigaddset(&myset, RTLINUX_SIGTIMER0)) != 0) {
		perror("rtlinux_sigaddset(&myset, irq)");
		return (-1);
	}

	/* setup for a test of the oneshot timer */
	sig.sa_handler = handler;
	sig.sa_flags = RTLINUX_SA_ONESHOT;
	sig.sa_period = NSECS_PER_SEC / 4;

	/* install our handler for oneshot */
	if (rtlinux_sigaction(RTLINUX_SIGTIMER0, &sig, &oldsig)) {
		printf("Couldn't get timer\n");
		perror("rtlinux_sigaction");
		return -1;
	}

	/* timer will go off, but be blocked/pended */
	if ((rtlinux_sigprocmask(RTLINUX_SIG_BLOCK, &myset, &oldset)) != 0) {
		perror
		    ("rtlinux_sigprocmask(RTLINUX_SIG_BLOCK, &myset, &oldset)");
		return (-1);
	}

	/* wait for the _timer_ to go off plus a bit extra in case
	 * oneshot is messing up and becoming periodic
	 */
	sleep(2);

	/* check that the _handler_ did *not* go off */
	if (*marker_ptr > 0) {
		fprintf(stderr,
			"oops, timer went off %d times when it was blocked\n",
			*marker_ptr);
		return -1;
	}

	/* timer will now be unblocked, so handler should run now */
	if ((rtlinux_sigprocmask(RTLINUX_SIG_UNBLOCK, &myset, &oldset)) !=
	    0) {
		perror
		    ("rtlinux_sigprocmask(RTLINUX_SIG_BLOCK, &myset, &oldset)");
		return (-1);
	}

	/* wait for the _handler_ to go off plus a bit extra in case
	 * oneshot is messing up and becoming periodic
	 */
	sleep(2);

	/* free the timer */
	sig.sa_handler = RTLINUX_SIG_IGN;
	rtlinux_sigaction(RTLINUX_SIGTIMER0, &sig, &oldsig);

	/* check that the timer went off once and only once */
	if (*marker_ptr != 1) {
		fprintf(stderr, "Marker should be 1, is %d for oneshot\n",
			*marker_ptr);
		return -1;
	}

	fprintf(stderr, "Testing periodic timer\n");

	/* setup for periodic */
	sig.sa_handler = handler;
	sig.sa_flags = RTLINUX_SA_PERIODIC;
	sig.sa_period = NSECS_PER_SEC / 4;
	*marker_ptr = 0;

	/* install the our periodic handler */
	if (rtlinux_sigaction(RTLINUX_SIGTIMER0, &sig, &oldsig)) {
		printf("Couldn't get timer\n");
		perror("rtlinux_sigaction");
		return -1;
	}

	/* wait for the signal to go off a few times */
	sleep(2);

	/* free the timer */
	sig.sa_handler = RTLINUX_SIG_IGN;
	rtlinux_sigaction(RTLINUX_SIGTIMER0, &sig, &oldsig);

	/* check that the timer went off the right number of times */
	if ((*marker_ptr < 7) || (*marker_ptr > 9)) {
		fprintf(stderr, "Marker should be 8, is %d for periodic\n",
			*marker_ptr);
		return -1;
	}

	fprintf(stderr, "Testing multiple periodic timers\n");

	/* setup 2 timers with different periods */
	*marker_ptr = 0;
	*marker2_ptr = 0;

	/* setup for periodic, at 2 different rates */
	sig.sa_handler = handler;
	sig.sa_flags = RTLINUX_SA_PERIODIC;
	sig.sa_period = NSECS_PER_SEC / 4;
	sig2.sa_handler = handler2;
	sig2.sa_flags = RTLINUX_SA_PERIODIC;
	sig2.sa_period = NSECS_PER_SEC / 8;

	/* install our periodic handlers */
	if (rtlinux_sigaction(RTLINUX_SIGTIMER0, &sig, &oldsig) ||
	    rtlinux_sigaction(RTLINUX_SIGTIMER1, &sig2, &oldsig)) {
		printf("Couldn't get timers\n");
		perror("rtlinux_sigaction");
		return -1;
	}

	/* wait for the handlers to run */
	sleep(2);

	/* free the timers */
	sig.sa_handler = RTLINUX_SIG_IGN;
	sig2.sa_handler = RTLINUX_SIG_IGN;
	rtlinux_sigaction(RTLINUX_SIGTIMER0, &sig, &oldsig);
	rtlinux_sigaction(RTLINUX_SIGTIMER1, &sig2, &oldsig);

	/* check that the timers went off the right number of times */
	if (((*marker_ptr < 7) || (*marker_ptr > 9))
	    && ((*marker2_ptr < 15) || (*marker_ptr > 17))) {
		fprintf(stderr, "Marker should be 8, is %d and Marker2"
			" should be 16 is %d\n", *marker_ptr,
			*marker2_ptr);
		return -1;
	}

	return 0;
}
