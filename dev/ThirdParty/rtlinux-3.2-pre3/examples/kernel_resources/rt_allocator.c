/*
 * Written by Der Herr Hofrat, der.herr@hofr.at
 * (C) 2002 FSMLabs
 * License: GPL Version 2
 */

#include <rtl.h>
#include <time.h>
#include <pthread.h>

static pthread_t rt_thread;


#include <linux/interrupt.h> /* for the tasklet macros/functions */
#include <linux/slab.h>      /* kmalloc */

void allocator_function(unsigned long arg);
#define BUFFERS 128

static char *iptr[BUFFERS]; /* static array of pointers for the buffers */
static int iptr_idx;

DECLARE_TASKLET(allocator_tasklet,allocator_function,0);


void
allocator_function(unsigned long arg)
{
	struct timeval now;
	do_gettimeofday(&now);
	printk("tasklet: allocating %ld at %ld,%ld\n",
		(unsigned long)arg,
		now.tv_sec,
		now.tv_usec);
	iptr[iptr_idx]=kmalloc((unsigned long)arg,
		GFP_ATOMIC);
	if(iptr[iptr_idx] == NULL){
		printk("tasklet: Allocation failed - out of memory\n");
	}
	else{
		memset(iptr[iptr_idx],
			0,
			(unsigned long)arg);
		printk("tasklet: Allocated 0'ed buffer %d (%ld bytes)\n",
			iptr_idx,
			(unsigned long)arg);
		iptr_idx++;
	}
	/* wake up the rt-thread that requested memory */
	pthread_kill(rt_thread,RTL_SIGNAL_WAKEUP);
}

unsigned long
rtl_kmalloc(unsigned long size)
{
	int idx;
	pthread_t self = pthread_self();
	RTL_MARK_SUSPENDED (self);
	rtl_printf("rtl_malloc: requesting %ld bytes\n",
		(unsigned long)size);
	/* if we are out of buffer pointers faile without calling the tasklet */
	idx = iptr_idx;
	if(idx < BUFFERS){
		allocator_tasklet.data=size;
		tasklet_hi_schedule(&allocator_tasklet);
		rtl_schedule();
		pthread_testcancel();
		if(iptr[idx] == NULL){
			return -1;
		}
		else{
			return idx;
		}
	}
	else{
		return -1;
	}
	return 0;
}

void * 
start_routine(void *arg)
{
	struct sched_param p;
	int ret;
	unsigned long i,size,block;
	p . sched_priority = 1;
	pthread_setschedparam (
		pthread_self(), 
		SCHED_FIFO, 
		&p);

	pthread_make_periodic_np (
		pthread_self(), 
		gethrtime(), 
		500000000);

	size=0;
	block=128;
	i=1;
	while (1) {
		pthread_wait_np ();
		size=block*i++;
		rtl_printf("RT-Thread; requesting %ld bytes of memory\n", 
			size);
		ret=rtl_kmalloc(size);
		/* apps must check that they actually got something */
		if(ret == -1){
			rtl_printf("No more buffers available\n");
		}
		else{
			rtl_printf("allocated buffer %d\n",ret);
		}
	}
	return 0;
}

int 
init_module(void)
{
	int i;
	for(i=0;i<BUFFERS;i++){
		iptr[i] = NULL;
	}
	return pthread_create (
		&rt_thread,
		NULL, 
		start_routine, 
		0);
}

void 
cleanup_module(void) {
	int i;
	for(i=0;i<BUFFERS;i++){
		if(iptr[i] != NULL){
			kfree(iptr[i]);
			printk("Freeing buffer %d\n",i);
		}
	}
	pthread_delete_np (rt_thread);
}
