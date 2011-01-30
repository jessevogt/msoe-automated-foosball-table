/*
 * Written by Der Herr Hofrat, der.herr@hofr.at
 * (C) 2002 FSMLabs
 * License: GPL Version 2
 */
#include <rtl.h>
#include <time.h>
#include <pthread.h>
#include "hello.h"

extern struct shared_mem_struct* shared_mem;

void
cleanup(void *arg)
{
	printk("Cleanup handler called\n");
}

void * start_routine(void *arg)
{
	struct sched_param p;
	p . sched_priority = 1;
	pthread_setschedparam (pthread_self(), SCHED_FIFO, &p);

	pthread_make_periodic_np (pthread_self(), gethrtime(), 500000000);
	pthread_cleanup_push(cleanup,0);

	while (1) {
		hrtime_t now;
		pthread_wait_np ();
		now = gethrtime();
		rtl_printf("I'm here; my shared mem=%d\n", 
			shared_mem->some_int);
	}
	pthread_cleanup_pop(0);
	return 0;
}
