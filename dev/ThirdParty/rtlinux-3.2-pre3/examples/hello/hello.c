#include <rtl.h>
#include <time.h>
#include <rtl_time.h>
#include <pthread.h>

pthread_t thread;
hrtime_t start_nanosec;


void * start_routine(void *arg)
{
	struct sched_param p;
	hrtime_t elapsed_time,now;
	p . sched_priority = 1;
	pthread_setschedparam (pthread_self(), SCHED_FIFO, &p);

	pthread_make_periodic_np (pthread_self(), gethrtime(), 500000000);

	while (1) {
		pthread_wait_np ();
		now = clock_gethrtime(CLOCK_REALTIME);
		elapsed_time = now - start_nanosec;
		rtl_printf("elapsed_time = %Ld\n",(long long)elapsed_time);
	}
	return 0;
}

int init_module(void) {
	start_nanosec = clock_gethrtime(CLOCK_REALTIME);
	return pthread_create (&thread, NULL, start_routine, 0);
}

void cleanup_module(void) {
	pthread_delete_np (thread);
}
