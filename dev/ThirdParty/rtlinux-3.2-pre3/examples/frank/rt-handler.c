/* vim: set ts=4: */
/*
 * Copywrite 2002 Der Herr Hofrat
 * License GPL V2
 * Author der.herr@hofr.at
 */
/*
 * example of using rtfifos to communicate between rt-threads
 */
#include <rtl.h>
#include <time.h>
#include <pthread.h>
#include <rtl_fifo.h>
#include <rtl_debug.h>

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Der Herr Hofrat <der.herr@hofr.at>");
MODULE_DESCRIPTION("example of using rtf_rt_handlers");

pthread_t p_thread,c_thread;

struct msg_t{
	unsigned long i;
	unsigned long something_else;
};

void * consumer(void *arg)
{
	struct sched_param p;
	int ret;
	struct msg_t msg;
	p.sched_priority = 1;
	pthread_setschedparam (pthread_self(), SCHED_FIFO, &p);

	msg.i=0;

	while (1) {
		pthread_wait_np();
		while ((ret = rtf_get(0,&msg,sizeof(msg))) == sizeof(msg)) {
			rtl_printf("consumer thread woken - got msg %ld\n",msg.i);
		}
	}
	return (void *)0;
}

void * producer(void *arg)
{
	struct sched_param p;
	struct msg_t msg;
	p . sched_priority = 1;
	pthread_setschedparam (pthread_self(), SCHED_FIFO, &p);
	pthread_make_periodic_np (pthread_self(), gethrtime(), 500000000);

	msg.i=0;
	msg.something_else=0xff;

	while (1) {
		pthread_wait_np();
		rtl_printf("producer sending msg %ld\n",msg.i);
		rtf_put(0,&msg,sizeof(msg));
		msg.i++;
	}
	return (void *)0;
}

int p_to_c(unsigned int fifo)
{
	/* wake up the consumer thread when data arived in rtf0 */
//	pthread_wakeup_np(c_thread);
	pthread_kill(c_thread,RTL_SIGNAL_WAKEUP);
	rtl_schedule();
	return 0;
}

int init_module(void) {
	int retval;

	/* the fifo to pass data from the producer to the consumer */
	rtf_destroy(0);
	rtf_create(0,4000);

	retval=pthread_create(&p_thread,NULL,producer, (void *)0);
	if(retval){
		printk("pthread create failed\n");
		rtf_destroy(0);
		return -1;
	}

	retval=pthread_create(&c_thread,NULL,consumer, (void *)0);
	if(retval){
		printk("pthread create failed\n");
		rtf_destroy(0);
		pthread_delete_np (p_thread);
		pthread_join(p_thread,(void *)&retval);
		return -1;
	}

	rtf_create_rt_handler(0, &p_to_c); 
	return 0;
}

void cleanup_module(void) {
	int retval;

	/* kill the producer first */
	pthread_delete_np (p_thread);
	pthread_join(p_thread,(void *)&retval);
	printk("producer thread terminated with %d\n",
		retval);

	/* now terminate the consumer */
	pthread_delete_np (c_thread);
	pthread_join(c_thread,(void *)&retval);
	printk("consumer thread terminated with %d\n", 
		retval);

	rtf_destroy(0);
}
