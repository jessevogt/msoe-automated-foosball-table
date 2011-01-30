/*
 * Written by Der Herr Hofrat, der.herr@hofr.at
 * (C) 2002 FSMLabs
 * License: GPL Version 2
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <rtl_time.h>
#include <rtl_sched.h>
#include <asm/io.h>
#include <time.h>
#include "common.h"

int period=1000000;
int periodic_mode=0;
int nibl=0xff; /* this nibl is 8bit :) - special nibl for rtlinux */

pthread_t thread;

void *
bit_toggle(void *t)
{
	pthread_make_periodic_np(thread, gethrtime(), period);

	while (1){
		outb(nibl,LPT);
		nibl = ~nibl;
 		pthread_wait_np();
		}
}

int 
init_module(void)
{
	pthread_attr_t attr;
	struct sched_param sched_param;

	pthread_attr_init (&attr);
	sched_param.sched_priority = 1;
	pthread_attr_setschedparam (&attr, &sched_param);
	pthread_create (&thread,  &attr, bit_toggle, (void *)0);

	return 0;
}

void 
cleanup_module(void)
{
	pthread_delete_np (thread);
}
