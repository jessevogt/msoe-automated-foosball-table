/* vim: set ts=4: */
/*
 * Copywrite 2002 Der Herr Hofrat
 * License GPL V2
 * Author der.herr@hofr.at
 */
/*
 * example of allocating 32MB of reserved memory to your rt-thread
 * reserve the memory with a mem= kernel commandline option.
 */
#include <rtl.h>
#include <time.h>
#include <pthread.h>

#include <asm/io.h> /* ioremap, iounmap */

pthread_t thread;

#define PERIOD 100000 /* default period 100us should be ok on all systems */
int period=PERIOD;
MODULE_PARM(period,"i"); 

/* total memory assumed to be 128MB */
unsigned long memstart=0x06000000;  /* 96MB */
MODULE_PARM(memstart,"l");
unsigned long memlength=0x02000000; /* 32MB */
MODULE_PARM(memlength,"l");

void *memptr;

void * start_routine(void *arg)
{
	struct sched_param p;
	unsigned long *data;
	unsigned long *memend;
	int i;
	time_t t1,t2;
	p . sched_priority = 1;
	pthread_setschedparam (pthread_self(), SCHED_FIFO, &p);

	pthread_make_periodic_np (pthread_self(), gethrtime(), period);

	memend=memptr+memlength;
	data=memptr;
	i=0;

	t1=time(NULL);	
	
	while (data < memend) {
		i++;
		pthread_wait_np();
		/* fill up the full 32MB with the index */
		*data++=(unsigned long)i;
	}

	t2=time(NULL);	

	rtl_printf("done at %d\n", (unsigned int)(t2-t1));
	return (void *)i;
}

int init_module(void) {
	/* rempa the top 32MB of physical RAM */
	printk("mapping %ld bytes at 0x%lx\n",memlength,memstart);
	memptr=ioremap(memstart,memlength);
	return pthread_create (&thread, NULL, start_routine, 0);
}

void cleanup_module(void) {
	void *thread_retval;
	pthread_cancel(thread);
	pthread_join(thread,&thread_retval);
	printk("thread terminated (return %d)\n",(int)thread_retval);

	/* releas the top 32MB of physical RAM */
	iounmap(memptr);
}
