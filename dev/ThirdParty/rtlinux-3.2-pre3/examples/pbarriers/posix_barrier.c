/*
 * posix_barrier.c
 *
 * Written by Patricia Balbastre <patricia@disca.upv.es>
 * Copyright (C) Dec, 2002 OCERA Consortium.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation version 2.
 *
 * RTLinux/GPL POSIX barrier example
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>

#include <rtl_sched.h>

#include <rtl.h>
#include <rtl_printf.h>
#include <rtl_time.h>
#include <rtl_barrier.h>

#include <time.h>

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Patricia Balbastre <patricia@disca.upv.es>, (OCERA)");
MODULE_DESCRIPTION("posix barrier usage example");

pthread_t t1,t2;

pthread_barrier_t   barrier; /* barrier synchronization object */

hrtime_t now;
#define DELAY 200000000

void *thread1 (void *not_used)
{
	struct timespec tv;
        clock_gettime(CLOCK_REALTIME,&tv);

	rtl_printf ("thread1 starting at %lld\n", gethrtime());

	/* do the computation
	 * let's just do a sleep here...
	 */
	timespec_add_ns(&tv,DELAY);
	clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &tv, NULL);
	pthread_barrier_wait (&barrier);

	/* after this point, all three threads have completed. */
	rtl_printf ("barrier in thread1() done  at %lld\n", gethrtime());

	return (void *)0;
}

void *thread2 (void *not_used)
{
	struct timespec tv;
        clock_gettime(CLOCK_REALTIME,&tv);

	rtl_printf ("thread2 starting at %lld\n", gethrtime());

	/* do the computation
	 * let's just do a sleep here...
	 */
	timespec_add_ns(&tv,(DELAY*2));
	clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &tv, NULL);
	pthread_barrier_wait (&barrier);

	/* after this point, all three threads have completed. */
	rtl_printf ("barrier in thread2() done  at %lld\n", gethrtime());

	return (void *)0;
}

int init_module(void)
{
	pthread_barrierattr_t barrier_attr;

	/* Initialize the barrier */
	pthread_barrierattr_init(&barrier_attr);
  
	/* create a barrier object with a count of 3 
	 * two rt-threads and init_module kernel context 
	 * should wait on this barrier
	 */
	pthread_barrier_init (&barrier, &barrier_attr, 3);

	/* don't reuse after destruction without 
	 * reinitialization with  pthread_barrierattr_init()
	 */
	pthread_barrierattr_destroy(&barrier_attr);
  
  	/* start up two threads, thread1 and thread2 
	 * using default attributes and priority
	 */
	pthread_create (&t1, NULL, thread1, NULL);
	pthread_create (&t2, NULL, thread2, NULL);
  
 	/* at this point, thread1 and thread2 are running
	 * now wait for completion...
	 */
	printk ("initmodule waiting for barrier at %lld\n",gethrtime());
	schedule_timeout(HZ); /* HZ jiffies = 1s */

	pthread_barrier_wait (&barrier);
  
	/* after this point, all three threads have completed. */
	printk ("barrier in initmodule done at %lld\n", gethrtime());

	return 0;
}

void cleanup_module(void)
{
	pthread_cancel(t1);
	pthread_join(t1,NULL);
	pthread_cancel(t2);
	pthread_join(t2,NULL);  

	pthread_barrier_destroy(&barrier);
}

