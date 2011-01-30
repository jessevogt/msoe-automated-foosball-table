#include <linux/errno.h>
#include <linux/kernel.h>
#include <rtl.h>
#include <time.h>

#include <rtl_sched.h>
#include <rtl_debug.h>
#include <rtl_fifo.h>
#include "bulk.h"

pthread_t sender;
int buffer[BUF_SIZE];
int infifo, outfifo;


void *thread_code(void *t)
{
	int n =0;
	int i = 0;

/* 	rtl_printf ("task starts on CPU %d\n", rtl_getcpuid()); */
	for(i=0; i< BUF_SIZE; i++)buffer[i]=i;

	while (n < COUNT*(BUF_SIZE*sizeof(int))) {
		i=rtf_put(outfifo, buffer,(BUF_SIZE*sizeof(int)));
		if(i <= 0){
			pthread_suspend_np(pthread_self());
			i=0;
		}
		else n += i;
	}


	n=0;
	while (n < COUNT*(BUF_SIZE*sizeof(int))) {
		i= rtf_get(infifo, buffer,BUF_SIZE*sizeof(int));
		if(i <= 0){
		       	pthread_suspend_np(pthread_self());
			i=0;
		}
		else n+= i;
	}
	pthread_suspend_np(pthread_self());

	return 0;
}


int my_handler(unsigned int fifo)
{
		pthread_wakeup_np (sender);
		return 0;
}




/* #define DEBUG  */
int init_module(void)
{
	pthread_attr_t attr;
	struct sched_param sched_param;
	int ret;
	int c_in,c_out;

	rtf_destroy(1);
	rtf_destroy(2);
 	c_in = rtf_create(1, 20*1024);
	infifo = 2;
 	c_out = rtf_create(2, 20*1024);
	outfifo = 1;
	if(c_in || c_out){
		printk("RTF bulk test fails to open fifos %d %d\n",c_in,c_out);
		return -1;
	}
	printk("Done with initializing fifos\n");

	pthread_attr_init (&attr);
	sched_param.sched_priority = 4;
	pthread_attr_setschedparam (&attr, &sched_param);
	ret = pthread_create (&sender,  &attr, thread_code, (void *)1);
	rtf_create_handler(1, &my_handler); 
	rtf_create_handler(2, &my_handler); 
	return 0;
}


void cleanup_module(void)
{
	rtf_destroy(1);
	rtf_destroy(2);
	pthread_cancel (sender);
	pthread_join (sender, NULL);
}
