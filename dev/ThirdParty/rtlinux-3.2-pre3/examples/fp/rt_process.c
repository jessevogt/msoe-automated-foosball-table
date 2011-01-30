/*
 * RTLinux FPU test example
 *
 * Written by Michael Barabanov, 1998
 *  (C) FSMLabs  1999. baraban@fsmlabs.com
 *  Released under the GNU GENERAL PUBLIC LICENSE Version 2, June 1991
 */

#include <linux/module.h>
#include <rtl.h>
#include <time.h>

#include <pthread.h>
#include <math.h>

pthread_t mytask;
pthread_t task2;
pthread_t task3;

/* this thread is marked as using FP with pthread attributes */
void *fun (void *t)
{
	int i = 0;
	double f = 0;
	while (1) {
		pthread_wait_np();
		i++;
		f++;
		if (i != f) {
			conpr("Task "); conprn ((int) t);
			conpr(" FP error: i = "); conprn(i);
			conpr("f = "); conprn(f); conpr("\n");
			i = 0;
			f = 0;
		}
		if (i > 200000) {
			i = 0;
			f = 0;
		}
	}
}


/* this thread is marked as using FP with pthread_setfp_np */
void *fun2 (void *t)
{
	int i = 0;
	double f;
	pthread_setfp_np(pthread_self(), 1);
	f = 0;
	while (1) {
		i++;
		f++;
		if (i != f) {
			conpr("Task "); conprn ((int) t);
			conpr(" FP error: i = "); conprn(i);
			conpr("f = "); conprn(f); conpr("\n");
			i = 0;
			f = 0;
		}
		if (i > 200000) {
			i = 0;
			f = 0;
		}
		pthread_wait_np();
	}
}



/* this thread does not use FPU */
void *ifun (void *t)
{
	int i=0;
	while (1) {
		pthread_wait_np();
		if (i++ % 5000 == 0) {
		}
	}
}


int init_module (void)
{
	struct sched_param p;
	pthread_attr_t attr;
	hrtime_t now = gethrtime();

	pthread_attr_init(&attr);
	pthread_attr_setfp_np(&attr, 1);

	pthread_create (&mytask, &attr, fun, (void *) 1);
	pthread_make_periodic_np (mytask, now + 2 * NSECS_PER_SEC, 31230000);
	p . sched_priority = 1;
	pthread_setschedparam (mytask, SCHED_FIFO, &p);

	pthread_create (&task2, NULL, fun2, (void *) 2);
	pthread_make_periodic_np (task2, now + 2 * NSECS_PER_SEC, 50000000);
	p . sched_priority = 2;
	pthread_setschedparam (task2, SCHED_FIFO, &p);

	pthread_create (&task3, NULL, ifun, (void *) 3);
	pthread_make_periodic_np (task3, now + 2 * NSECS_PER_SEC, 30000000);
	p . sched_priority = 3;
	pthread_setschedparam (task3, SCHED_FIFO, &p);

	return 0;
}


void cleanup_module (void)
{
	pthread_delete_np (mytask);
	pthread_delete_np (task2);
	pthread_delete_np (task3);
}

