/*
 * based on rtlsigtimer_app.c
 *
 * tests multiple psc-threads that can interrupt each other
 *
 * GPL v2 (c) by Nils Hasler 2003
 *
 */

#include <stdio.h>
#include <rtlinux_signal.h>
#include <sys/times.h>

volatile unsigned long marker = 0, marker2 = 0;
volatile int busy = 0;

void handler(int unused)
{
	if(busy)
		marker2++;
	marker++;
}

/*
 * this handler wastes lots of cpu-cycles
 */
void longhandler(int unused)
{
	int i, n = 1;
	busy = 1;

	/* waste cycles */
	for(i = 0; i < 100000000; i++)
		n *= 13;

	busy = 0;
}

int main(void)
{
	struct rtlinux_sigaction sig, sig2, oldsig;
	rtlinux_sigset_t myset, oldset;
	clock_t starttime;
	int delta;

	fprintf(stderr, "Testing compatibility\n");
	/* setup for a test of timer signal masks */
	if ((rtlinux_sigemptyset(&myset)) != 0) {
		perror("rtlinux_sigemptyset(&myset)");
		return (-1);
	}

	if ((rtlinux_sigaddset(&myset, RTLINUX_SIGTIMER0)) != 0) {
		perror("rtlinux_sigaddset(&myset, irq)");
		return (-1);
	}

	/* the first thread is not interruptible but the second is */
	sig.sa_handler = longhandler;
	sig.sa_flags = RTLINUX_SA_ONESHOT;
	sig.sa_period = NSECS_PER_SEC / 4;
	sig2.sa_handler = handler;
	sig2.sa_flags = RTLINUX_SA_PERIODIC | RTLINUX_SA_MULTIPSC;
	sig2.sa_period = NSECS_PER_SEC / 8;
	sig2.sa_priority = 10;

	/* install our handlers */
	if (rtlinux_sigaction(RTLINUX_SIGTIMER0, &sig, &oldsig) ||
	    rtlinux_sigaction(RTLINUX_SIGTIMER1, &sig2, &oldsig)) {
		printf("Couldn't get timers\n");
		perror("rtlinux_sigaction");
		return -1;
	}

	/* wait for the handlers to run */
	sleep(1);

	/* free the timers */
	sig.sa_handler = RTLINUX_SIG_IGN;
	sig2.sa_handler = RTLINUX_SIG_IGN;
	rtlinux_sigaction(RTLINUX_SIGTIMER0, &sig, &oldsig);
	rtlinux_sigaction(RTLINUX_SIGTIMER1, &sig2, &oldsig);

	/* check that the long handler was not interrupted */
	if(marker2) {
		fprintf(stderr, "Marker2 should be 0, is %d\n", marker2);
		return -1;
	}

	fprintf(stderr, "Testing priorities\n");

	marker = 0;
	marker2 = 0;
	busy = 0;

	/* the first thread has a higher priority */
	sig.sa_handler = longhandler;
	sig.sa_flags = RTLINUX_SA_ONESHOT | RTLINUX_SA_MULTIPSC;
	sig.sa_period = NSECS_PER_SEC / 4;
	sig.sa_priority = 20;
	sig2.sa_handler = handler;
	sig2.sa_flags = RTLINUX_SA_PERIODIC | RTLINUX_SA_MULTIPSC;
	sig2.sa_period = NSECS_PER_SEC / 8;
	sig2.sa_priority = 10;

	/* install our handlers */
	if (rtlinux_sigaction(RTLINUX_SIGTIMER0, &sig, &oldsig) ||
	    rtlinux_sigaction(RTLINUX_SIGTIMER1, &sig2, &oldsig)) {
		printf("Couldn't get timers\n");
		perror("rtlinux_sigaction");
		return -1;
	}

	/* wait for the handlers to run */
	sleep(1);

	/* free the timers */
	sig.sa_handler = RTLINUX_SIG_IGN;
	sig2.sa_handler = RTLINUX_SIG_IGN;
	rtlinux_sigaction(RTLINUX_SIGTIMER0, &sig, &oldsig);
	rtlinux_sigaction(RTLINUX_SIGTIMER1, &sig2, &oldsig);

	/* check that the long handler was not interrupted */
	if(marker2) {
		fprintf(stderr, "Marker2 should be 0, is %d\n", marker2);
		return -1;
	}

	fprintf(stderr, "Testing interruptibility\n");

	marker = 0;
	marker2 = 0;
	busy = 0;

	/* the second thread has a higher priority and should interrupt the first one */
	sig.sa_handler = longhandler;
	sig.sa_flags = RTLINUX_SA_ONESHOT | RTLINUX_SA_MULTIPSC;
	sig.sa_period = NSECS_PER_SEC / 4;
	sig.sa_priority = 10;
	sig2.sa_handler = handler;
	sig2.sa_flags = RTLINUX_SA_PERIODIC | RTLINUX_SA_MULTIPSC;
	sig2.sa_period = NSECS_PER_SEC / 8;
	sig2.sa_priority = 20;

	/* install our handlers */
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

	/* check that the long handler was interrupted */
	if(marker2 == 0) {
		fprintf(stderr, "Marker2 should not be 0, is 0\n");
		return -1;
	}

	return 0;
}

