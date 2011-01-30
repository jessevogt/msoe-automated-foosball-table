/*
 * Written by Der Herr Hofrat, der.herr@hofr.at
 * (C) 2002 FSMLabs
 * License: GPL Version 2
 */

#include <rtl.h>
#include <time.h>
#include <pthread.h>
#include <asm/io.h>
#include "common.h"

/* set the paralell port address */
int lpt_port=0x378;

/* compiled in addresses are unpractical so lsets define a module parameter 
 * that permits setting the parport address at module insertion time 
 * insmod sched_toggle.o lpt_port=0x3bc would override 0x378 .
 */
MODULE_PARM(lpt_port,"i");

pthread_t thread;

void * 
toggle(void *arg)
{
	int nibl;
        hrtime_t now;
	struct sched_param p;
	p . sched_priority = 1;

        /* set the attributes for the thread , this 
         * defines the scheduling policy and priority
         */
	pthread_setschedparam (pthread_self(), SCHED_FIFO, &p);

        now = gethrtime(); /* get the current time in nano-seconds */

        /* make the thread periodic 
         * starting now , with a period of 500000000 nano-seconds
         */ 
	pthread_make_periodic_np (pthread_self(), now, 500000000);

        nibl = 0x0f;
	while (1) {
                outb(nibl,lpt_port); /* write it out to the parport*/
                nibl = ~nibl;        /* two's complement of nibl*/
		pthread_wait_np ();  /* put the thread on the wait queue */
	}
	return 0;
}

/* init_module is called when the module is inserted it will do 
 * the basic setup and register the module with the kernel
 */
int 
init_module(void)
{
	rtl_printf("RTL thread starts on CPU%d : using LPT at 0x%x\n",
		rtl_getcpuid(),lpt_port);

        /* create the thread                                       
         * attributes are "initialized" to NULL and set in toggle 
         *
         *                      thread, attributes, function , ARG
         */
	return pthread_create (&thread, NULL      , toggle   , 0  );
}

/* when the module is removed with  rmmod sched_toggle  this is called
 * to cleanup any kernels trace of this module
 */
void 
cleanup_module(void)
{
        /* send a request for cancelation*/
	pthread_delete_np (thread);
}
