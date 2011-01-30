/*
 * (C) Finite State Machine Labs Inc. 1999-2000 business@fsmlabs.com
 *
 * Released under the terms of GPL 2.
 * Open RTLinux makes use of a patented process described in
 * US Patent 5,995,745. Use of this process is governed
 * by the Open RTLinux Patent License which can be obtained from
 * www.fsmlabs.com/PATENT or by sending email to
 * licensequestions@fsmlabs.com
 */

/*
 * PPC actually died on this at one point so we test for it
 * now... -- Cort
 */
int main(void)
{
	long long i;
	unsigned long foo;

	while (1) {
		for (i = 0; i < 1000000000; i++);
		asm volatile ("mfmsr %0":"=r" (foo));
		printf("BEEP\n");
	}
	/* nothing */ ;
	return 0;
}
