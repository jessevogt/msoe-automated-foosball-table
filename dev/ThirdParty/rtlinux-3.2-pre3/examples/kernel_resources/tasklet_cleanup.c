/*
 * Copywrite 2002 Der Herr Hofrat
 * License GPL V2
 * Author der.herr@hofr.at
 */
/*
 * example of cleaning up a tasklet properly for
 * asynchronous termination cases via cleanup handler
 */ 
#include <rtl.h>
#include <time.h>
#include <linux/interrupt.h> /* for the tasklet macros/functions */
#include <pthread.h>

int myint_for_something=1;
pthread_t thread;

void tasklet_function(unsigned long);

char tasklet_data[64];

DECLARE_TASKLET(test_tasklet,tasklet_function, (unsigned long) &tasklet_data);
	
void 
tasklet_cleanup(void *arg){
	tasklet_enable(&test_tasklet);
	rtl_printf("cleanup handler called\n");
}

void * start_routine(void *arg)
{
	struct sched_param p;
	int i;
	p . sched_priority = 1;
	pthread_setschedparam (pthread_self(), SCHED_FIFO, &p);

	pthread_make_periodic_np (pthread_self(), gethrtime(), 500000000);

	/* if rmmod gets called with the tasklet scheduled but disabled the
	 * box will freez - make shure that tasklets are enabled on exit 
 	 * module via a cleanup handler.
	 */
	pthread_cleanup_push(tasklet_cleanup,0);
	i=0;

	while (1) {
		pthread_wait_np ();
		rtl_printf("RT-Thread; my arg is %x\n", (unsigned) arg);
		sprintf(tasklet_data,"%s \"%x\"",
			"Linux tasklet received RT-Thread arg",
			(unsigned) arg);
		if(i==5){
			tasklet_disable(&test_tasklet);
			rtl_printf("killed tasklet\n");
		}
		tasklet_hi_schedule(&test_tasklet);
		i++;
	}
	pthread_cleanup_pop(0);
	return 0;
}
	
void tasklet_function(unsigned long data){
	struct timeval now;
	do_gettimeofday(&now);
	printk("%s at %ld,%ld\n",(char *) data,now.tv_sec,now.tv_usec);
}
	
int init_module(void) {
	return pthread_create (&thread, NULL, start_routine, 0);
}

void cleanup_module(void) {
	pthread_delete_np(thread);
}
