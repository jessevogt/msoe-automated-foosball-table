/*
 * Written by Der Herr Hofrat, der.herr@hofr.at
 * (C) 2002 FSMLabs
 * License: GPL Version 2
 */

//#include <asm/softirq.h>
#include <rtl.h>
#include <time.h>
#include <pthread.h>
#include <rtl_fifo.h>
#include <unistd.h>
#include "common.h"

static pthread_t thread;
static int my_softirq;

hrtime_t call_time;
hrtime_t last_time=0;
hrtime_t max_diff=-200000;
hrtime_t min_diff=200000;

int ntests=500;
int count=0;
int fifo_size=4096;
int rtf_fd;

#define PERIOD 1000000

extern asmlinkage void do_softirq(void);

void * 
start_routine(void *arg)
{
	hrtime_t abstime = clock_gethrtime(CLOCK_REALTIME) + 1000000000;

	struct sched_param p;
	p . sched_priority = 1;
	pthread_setschedparam (pthread_self(), SCHED_FIFO, &p);

	rtf_fd = open("/dev/rtf0", O_NONBLOCK);
	if (rtf_fd < 0) {
		printk("/dev/rtf0 open returned %d\n", rtf_fd);
		return (void *)-1;
	}

	while (1) {
		clock_nanosleep (CLOCK_REALTIME, TIMER_ABSTIME,
			hrt2ts(abstime), NULL);
		call_time = clock_gethrtime(CLOCK_REALTIME);
		rtl_global_pend_irq(my_softirq);
		do_softirq();
		abstime += PERIOD;
	}
	return 0;
}

static void 
my_handler(int irq,void *ignore,struct pt_regs *ignoreregs)
{
	hrtime_t diff;
	struct sample samp;
	hrtime_t now = clock_gethrtime(CLOCK_REALTIME);

	diff = now - call_time;
	if( count < ntests){
		if( diff > max_diff ){
			max_diff = diff;
		}
		if( diff < min_diff ){
			min_diff = diff;
		}
	}
	else
	{
		samp.min = min_diff;
		samp.max = max_diff;
//		printk("min: %8d, max: %8d\n",(int)min_diff,(int)max_diff);
		write(rtf_fd,&samp,sizeof(samp));
		count=0;
		max_diff=-200000;
		min_diff=200000;
	}
	count++;
}

int 
init_module(void)
{
	int ret;
	rtf_destroy(0);
	ret=rtf_create(0,fifo_size);
	if(ret){
		printk("rtf_create failed\n");
		return -1;
	}
	ret=pthread_create(
		&thread,
		NULL,
		start_routine,
		0);
	if(ret){
		printk("pthread_create failed\n");
		return -1;
	}
	my_softirq=rtl_get_soft_irq(my_handler,"sofirq jitter test");
	if(ret < 0){ /* actually irq 0 never can be legal ither...*/
		printk("rtl_get_soft_irq failed with %d\n",-ret);
		return -ret;
	}
	return 0;
}

void 
cleanup_module(void) 
{
	rtl_free_soft_irq(my_softirq);
	pthread_cancel(thread);
	pthread_join(thread,NULL);
	close(rtf_fd);
	rtf_destroy(0);
}
