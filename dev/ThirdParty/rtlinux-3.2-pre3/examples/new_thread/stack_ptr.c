/*
 * Written by Der Herr Hofrat, der.herr@hofr.at
 * (C) 2002 FSMLabs
 * License: GPL Version 2
 */

#include <rtl.h>
#include <time.h>
#include <pthread.h>
#include <linux/malloc.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Der Herr Hofrat <der.herr@hofr.at>");
MODULE_DESCRIPTION("pthread_create from within rt-context example");

static pthread_t thread1,new_thread;
static int new_thread_created=0;

void * 
new_routine(void *arg)
{
	struct sched_param p;
	p . sched_priority = 1;
	pthread_setschedparam (pthread_self(), SCHED_FIFO, &p);

	pthread_make_periodic_np (pthread_self(), gethrtime(), 800000000);

	while (1) {
		pthread_wait_np ();
		rtl_printf("New thread; my arg is %x\n", (unsigned) arg);
	}
	return 0;
}

void * 
start_routine(void *arg)
{
	int ret;
	struct sched_param p;
	pthread_attr_t new_attr;
	int stack_size;
	p . sched_priority = 1;
	pthread_setschedparam (pthread_self(), SCHED_FIFO, &p);

	pthread_make_periodic_np(pthread_self(), gethrtime(),500000000);

	pthread_attr_init(&new_attr);
	stack_size = new_attr.stack_size; /* default stack size */
	/* musst use GFP_ATTOMIC, GFP_KERNEL may sleep */
	new_attr.stack_addr = (int *)kmalloc(stack_size,GFP_ATOMIC);
	if(!new_attr.stack_addr){
		rtl_printf("kmalloc failed - exiting\n");
		return (void *)-1;
	}
	ret=pthread_create(&new_thread,&new_attr,new_routine, (void *)66);
	if(ret == EAGAIN){
		rtl_printf("pthread_create failed\n");
	}
	else{
		rtl_printf("pthread_create success (return = %d)\n",ret);
		new_thread_created=1; /* tell cleanup_module about it */
	}

	while (1) {
		pthread_wait_np ();
		rtl_printf("Old thread; my arg is %x\n",(unsigned)arg);
	}
	return 0;
}
	
int 
init_module(void) 
{
	return pthread_create (&thread1, NULL, start_routine, 0);
}

void 
cleanup_module(void) 
{
	pthread_delete_np (thread1);
	/* don't forget to remove the newly created thread 
	 */
	if(new_thread_created){
		pthread_delete_np (new_thread);
		printk("new thread deleted\n");
	}
}
