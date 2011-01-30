#include <rtl.h>
#include <time.h>
#include <pthread.h>

pthread_t thread;

void * start_routine(void *arg)
{
	int ret;
	hrtime_t abstime;

	struct sched_param p;
	p . sched_priority = 1;
	pthread_setschedparam (pthread_self(), SCHED_FIFO, &p);

	pthread_make_periodic_np (pthread_self(), clock_gethrtime(CLOCK_REALTIME), 4000000000LL);

	while (1) {
		pthread_wait_np ();
		rtl_printf("I'm here; my arg is %p\n", arg);

		ret = nanosleep(hrt2ts(500000000), NULL);
		rtl_printf("    Here I come again (%d)\n", ret);

		abstime = clock_gethrtime(CLOCK_REALTIME) + 1000000000;
		ret = clock_nanosleep (CLOCK_REALTIME, TIMER_ABSTIME,
			hrt2ts(abstime), NULL);

		rtl_printf("and again (%d)\n", ret);

		ret = clock_nanosleep (CLOCK_REALTIME, 0,
				hrt2ts(1000000000), NULL);
		rtl_printf("            and again still (%d)\n", ret);

	}
	return 0;
}

int init_module(void) {
	return pthread_create (&thread, NULL, start_routine, 0);
}

void cleanup_module(void) {
	pthread_delete_np (thread);
}
