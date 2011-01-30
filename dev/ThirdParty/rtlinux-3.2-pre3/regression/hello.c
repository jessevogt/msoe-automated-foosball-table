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

#include <rtl.h>
#include <rtl_fifo.h>
#include <time.h>
#include <pthread.h>

pthread_t thread;
int i = 0;

void *start_routine(void *arg)
{
	struct sched_param p;
	p.sched_priority = 1;
	pthread_setschedparam(pthread_self(), SCHED_FIFO, &p);

	pthread_make_periodic_np(pthread_self(), gethrtime(), 500000000);

	while (1) {
		pthread_wait_np();
		i++;
		rtf_put(0, "I'm still here", strlen("I'm still here"));
	}
	return 0;
}

int init_module(void)
{
	pthread_create(&thread, NULL, start_routine, 0);
	rtf_create(0, 8192);
	return 0;
}

void cleanup_module(void)
{
	pthread_delete_np(thread);
	rtf_destroy(0);
}
