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

unsigned long marker = 0;
volatile unsigned long *marker_ptr = &marker;

void handler(int unused)
{
	*marker_ptr = 1;
}

int main(int argc, char **argv)
{
	int i = 10;
	struct rtlinux_sigaction sig, oldsig;
	rtlinux_sigset_t myset, oldset;
	int irq = 0;

	if (argc != 2) {
		fprintf(stderr, "%s IRQ#\n", *argv);
		return -1;
	}

	irq = atoi(argv[1]);
	fprintf(stderr, "Waiting for irq %d\n", irq);

	if ((rtlinux_sigemptyset(&myset)) != 0) {
		perror("rtlinux_sigemptyset(&myset)");
		return (-1);
	}

	if ((rtlinux_sigaddset(&myset, irq)) != 0) {
		perror("rtlinux_sigaddset(&myset, irq)");
		return (-1);
	}

	sig.sa_handler = handler;
	sig.sa_flags = RTLINUX_SA_ONESHOT;

	if (rtlinux_sigaction(irq, &sig, &oldsig)) {
		printf("Couldn't get irq\n");
		perror("rtlinux_sigaction");
		return -1;
	}

	if ((rtlinux_sigprocmask(RTLINUX_SIG_BLOCK, &myset, &oldset)) != 0) {
		perror
		    ("rtlinux_sigprocmask(RTLINUX_SIG_BLOCK, &myset, &oldset)");
		return (-1);
	}

	*marker_ptr = 0;

#ifdef RTL_PSC_SIGMASKS_WORK_CORRECTLY
	while (i--) {
		sleep(1);
		if (*marker_ptr) {
			/* free the irq */
			sig.sa_handler = RTLINUX_SIG_IGN;
			rtlinux_sigaction(irq, &sig, &oldsig);
			fprintf(stderr,
				"oops, handler got called at wrong time\n");
			return -1;
		}
	}
#endif

	if ((rtlinux_sigprocmask(RTLINUX_SIG_UNBLOCK, &myset, &oldset)) !=
	    0) {
		perror
		    ("rtlinux_sigprocmask(RTLINUX_SIG_BLOCK, &myset, &oldset)");
		return (-1);
	}

	i = 10;

	while (i--) {
		sleep(1);
		if (*marker_ptr) {
			/* free the irq */
			sig.sa_handler = RTLINUX_SIG_IGN;
			rtlinux_sigaction(irq, &sig, &oldsig);
			return 0;
		}
	}

	/* free the irq */
	sig.sa_handler = RTLINUX_SIG_IGN;
	rtlinux_sigaction(irq, &sig, &oldsig);
	printf("marker unchanged\n");
	return -1;
}
