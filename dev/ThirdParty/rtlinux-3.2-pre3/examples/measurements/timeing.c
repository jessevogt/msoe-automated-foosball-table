/* vim: set ts=4: */
/*
 * Copywrite 2002 Der Herr Hofrat
 * License GPL V2
 * Author der.herr@hofr.at
 */
/*
 * very basic timing measurement module 
 *
 * - first it checks the minimum timedifference that the system
 * can register - this can ither be limited by the numeric granularity of
 * the gethrtime function (generally the case on fast PIII/P4 systems) or
 * by the execution time of the gethrtime function it selfe
 *
 * - second we check the time-stamp precision of the system with disabled 
 * interrupts. Note though that this value is only relevant if you
 * are running your task with disabled interrupts.
 *
 * - third with enabled interrupts the timedifference between two
 * consecutive calls to gethrtime is recorded - this naturally will approach
 * a maximum that is dependant on system load and must run for quite some time
 * to be a stable value - the de-facto limiting factor for the timestamp 
 * precision is the runtime of the interrupt emulation code - so no time value
 * recorded can be taken as more reliable than the generally 10-15us that this
 * loop will report. If one has hard real-time interrupt service routines
 * registered then this value will depend on the execution time of the emulation * code + your ISR !
 *
 * Note: the max-min in the rtl_printf statements are due to the fact that
 *       you can compensate the time measurement error by atleast the 
 *       value found as time granularity - in some cases better compensation
 *       may also be posible.
 */

#include <rtl.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("der.herr@hofr.at");
MODULE_DESCRIPTION("basic timeing measurement");

#include <rtl.h>
#include <time.h>
#include <rtl_time.h>
#include <pthread.h>
#include <rtl_spinlock.h>

pthread_t thread;
hrtime_t start_nanosec;
static pthread_spinlock_t lockloop;


void * start_routine(void *arg)
{
	struct sched_param p;
    int i;
	p . sched_priority = 1;
	pthread_setschedparam (pthread_self(), SCHED_FIFO, &p);

   hrtime_t max,min,first,second,diff;

   min=200000; /* some large number */
   max=0;

   pthread_spin_lock (&lockloop);
   for(i=0;i<10000;i++){
      first=gethrtime();
      second=gethrtime();
		diff=second-first;
      if(diff<min){
          min=diff;
	  }
	}
    pthread_spin_unlock (&lockloop);
	rtl_printf("gethrtime resolution is %ld ns\n",min);

   /* get the time stamp precision of the uninterrupted system*/
   pthread_spin_lock (&lockloop);
   for(i=0;i<1000000;i++){
      first=gethrtime();
      second=gethrtime();
      diff=second-first;
      if(diff>max){
          max=diff;
      }
   }
   pthread_spin_unlock (&lockloop);
   rtl_printf("gethrtime time stamp precision %ld ns (interrupts disabled)\n",max-min);

	pthread_make_periodic_np (pthread_self(), gethrtime(), 500000000);

	while (1) {
		pthread_wait_np();
	    for(i=0;i<10000;i++){
		    first=gethrtime();
			second=gethrtime();
			diff=second-first;
			if(diff>max){
				max=diff;
			}
		}
   	rtl_printf("gethrtime time stamp precision %ld nanoseconds\n",max-min);
	}
	return 0;
}

int init_module(void) {
    pthread_spin_init (&lockloop, 0);
    start_nanosec = clock_gethrtime(CLOCK_REALTIME);
    return pthread_create (&thread, NULL, start_routine, 0);
}

void cleanup_module(void) {
    pthread_delete_np (thread);
    pthread_spin_destroy(&lockloop);
}
