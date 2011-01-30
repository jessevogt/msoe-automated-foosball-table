/*
 * Periodic timing measurement for RTLinux.
 *
 * Copyright (C) 1999 FSM Labs (http://www.fsmlabs.com/)
 *  Written by Cort Dougan <cort@fsmlabs.com>
 */

#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <rtl_fifo.h>
#include <rtl_time.h>

struct samp
{
	hrtime_t min, max, total;
	hrtime_t period;
	int cnt;
};

int main()
{
	int fd0;
	int n;
	struct samp sm;
	FILE *f;
	int i = 0;
	
	if ((fd0 = open("/dev/rtf0", O_RDONLY)) < 0)
	{
		fprintf(stderr, "Error opening /dev/rtf0\n");
		exit(1);
	}

	if ( (f = fopen("gnuplot.out", "w")) == NULL )
	{
		fprintf(stderr,"Cannot open gnuplot.out\n");
		perror("open");
		exit(-1);
	}

	while (i < 50)
	{
		n = read(fd0, &sm, sizeof(sm));
#define NS_TO_US(x) ((x)/(NSECS_PER_SEC/1000000))
		printf("min: %5ldus \tavg: %5ldus \tmax: %5ldus \tperiod: %5ldus\n",
		       (ulong)NS_TO_US((int)sm.min-(int)sm.period),
		       (ulong)NS_TO_US(((int)sm.total/(int)sm.cnt)-(int)sm.period),
		       (ulong)NS_TO_US((int)sm.max-(int)sm.period),
		       (ulong)NS_TO_US((int)sm.period));
		fflush(stdout);
		i++;
		fprintf(f, "%d %ld %5ld %5ld %5ld\n",
			i,
			(ulong)NS_TO_US((int)sm.min/*-sm.period*/),
			(ulong)NS_TO_US(((int)sm.total/(int)sm.cnt)/*-sm.period*/),
			(ulong)NS_TO_US((int)sm.max/*-sm.period*/),
			(ulong)NS_TO_US((int)sm.period));
		fflush(f);
	}

	return 0;
}
