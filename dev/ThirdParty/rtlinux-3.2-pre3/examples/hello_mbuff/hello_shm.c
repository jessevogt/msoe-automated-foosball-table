/*
 * Written by Der Herr Hofrat, der.herr@hofr.at
 * (C) 2002 FSMLabs
 * License: GPL Version 2
 */
/*
 * RTLinux "Hello World" example using schared memmory (mbuff)
 */

#include <rtl.h>
#include <pthread.h>
#include "hello_shm.h"

pthread_t thread;

void * 
start_routine(void *arg)
{
	hrtime_t next=clock_gethrtime(CLOCK_REALTIME) + 1000000000;

	while(1){
		clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME,
			hrt2ts(next), NULL);
		next += 500000000;
		rtl_printf("I'm here; found `%s` in hello_shm\n", 
			(char *)hello_shm);
	}
	return 0;
}

int 
init_module(void) 
{
	int ret;
	hello_shm=(volatile char*) mbuff_alloc("hello_shm",1024);
	if(hello_shm == NULL) {
		printk("mbuff_alloc failed\n");
		return -1;	
	}
	sprintf((char*)hello_shm,"hello world");
	printk("Init module wrote `%s` to hello_shm\n", (char*)hello_shm);

	ret = pthread_create (&thread, NULL, start_routine, 0);
	if(ret != 0){
		printk("Failed to create RT-thread: %d\n",ret);
		mbuff_free("hello_shm",(void*)hello_shm);
		return -1;
	}
	return 0;
}

void 
cleanup_module(void)
{
	pthread_cancel (thread);
	pthread_join (thread, NULL);

	mbuff_free("hello_shm",(void*)hello_shm);
}
