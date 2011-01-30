#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <rtlinux_signal.h>

#define SAMPLES_PER_ROUND 50
#define ROUNDS 50

#undef TEST_LINUX

hrtime_t times[SAMPLES_PER_ROUND];
volatile int testnum = 0;
hrtime_t period = 30000 * 1000;

void rtlinux_handler(int whatever)
{
	times[testnum++] = gethrtime();
}

#ifdef TEST_LINUX
void linux_handler(void)
{
	struct timeval tv;
	while (testnum < SAMPLES_PER_ROUND )
	{
		gettimeofday(&tv,NULL);
		times[testnum++] = (hrtime_t)tv.tv_usec*1000;
		usleep(period/1000);
	}
}
#endif

int main(void)
{
	hrtime_t diff;
	hrtime_t min = HRTIME_INFINITY, max = 0, total = 0;
	struct rtlinux_sigaction mysig, oldsig;
	int i, j;
	FILE *f;
	
	if ( (f = fopen("gnuplot.out", "w")) == NULL )
	{
		fprintf(stderr,"Cannot open gnuplot.out\n");
		perror("open");
		exit(-1);
	}

	/* for each of the testing rounds */
	for ( i = 0; i < ROUNDS ; i++ )
	{
		/* test RTLinux first */

		/* setup the handler for our timer */
		mysig.sa_handler = rtlinux_handler;
		mysig.sa_flags = RTLINUX_SA_PERIODIC;
		mysig.sa_period = period;
		if ((rtlinux_sigaction(RTLINUX_SIGTIMER0, &mysig, &oldsig)))
		{
			perror("rtlinux_sigaction");
			return (-1);
		}

		/* wait for the tests to run */
		while ( testnum < SAMPLES_PER_ROUND )
			usleep(1000);

		/* free the handler */
		mysig.sa_handler = RTLINUX_SIG_IGN;
		if ((rtlinux_sigaction(RTLINUX_SIGTIMER0, &mysig, &oldsig)))
		{
			perror("rtlinux_sigaction");
			return (-1);
		}

		for ( j = 1; j < SAMPLES_PER_ROUND; j++ )
		{
			if (max < (times[j] - times[j-1]) )
				max = times[j] - times[j-1];
			if (min > (times[j] - times[j-1]) )
				min = times[j] - times[j-1];
			total += times[j] - times[j-1];
		}
		fprintf(f ,"%d %ld %ld %ld ",
			i, (long)min/1000, (long)((total/(SAMPLES_PER_ROUND-1))/1000),
			(long)max/1000);
		printf("RTLinux min: %ld us avg: %ld us max: %ld us\n",
		       (long)(min/1000), (long)((total/(SAMPLES_PER_ROUND-1))/1000),
		       (long)(max/1000));

		max = total = 0;
		min = HRTIME_INFINITY;
		testnum = 0;

#ifdef TEST_LINUX
		/* test Linux */
		linux_handler();
		for ( j = 1; j < SAMPLES_PER_ROUND; j++ )
		{
			hrtime_t diff = times[j] - times[j-1];
			/* counter rolled */
			if ( diff < 0 )
				diff = times[j] + ((hrtime_t)1000000000 - times[j-1]);
			if (max < diff )
				max = diff;
			if (min > diff)
				min = diff;
			total += diff;
		}

		fprintf(f ,"%ld %ld %ld %ld",
			(long)min/1000, (long)((total/(SAMPLES_PER_ROUND-1))/1000),
			(long)max/1000, (long)period/1000);
		fflush(f);
		printf("Linux min: %ld avg: %ld us max: %ld us\n",
		       (long)(min/1000), (long)((total/(SAMPLES_PER_ROUND-1))/1000),
		       (long)(max/1000));

#endif /* TEST_LINUX */
		fprintf(f, "\n");
		fflush(f);
		
		max = total = 0;
		min = HRTIME_INFINITY;
		testnum = 0;
	}

	fclose(f);
	return 0;
}
