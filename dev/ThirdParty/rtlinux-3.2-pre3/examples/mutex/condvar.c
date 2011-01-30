#include <rtl.h>
#include <time.h>
#include <pthread.h>

static pthread_t thread;
static pthread_mutex_t mutex /* = PTHREAD_MUTEX_INITIALIZER */;
static pthread_cond_t cond;
static int timed=1;
MODULE_PARM(timed,"i");

static void * start_routine(void *arg)
{
	int ret;
	hrtime_t t;
	hrtime_t t2;

	struct sched_param p;
	p . sched_priority = 1;
	pthread_setschedparam (pthread_self(), SCHED_FIFO, &p);

	rtl_printf("RTLinux thread starts on CPU%d\n", rtl_getcpuid());
	if (timed) {
		rtl_printf("RT-thread: about to pthread_cond_timedwait\n");
		t = clock_gethrtime(CLOCK_REALTIME);

		pthread_mutex_lock (&mutex);
		ret = pthread_cond_timedwait (&cond, &mutex, hrt2ts(t + 2000000));
		pthread_mutex_unlock (&mutex);

		t2 = clock_gethrtime(CLOCK_REALTIME);

		rtl_printf("RT-thread: pthread_cond_timedwait returned %d (%d ns elapsed)\n", ret, (unsigned) (t2 - t));
	}


	rtl_printf("RT-thread: about to pthread_cond_wait\n");
	t = clock_gethrtime(CLOCK_REALTIME);

	pthread_mutex_lock (&mutex);
	ret = pthread_cond_wait (&cond, &mutex);
	pthread_mutex_unlock (&mutex);

	t2 = clock_gethrtime(CLOCK_REALTIME);

	rtl_printf("RT-thread: pthread_cond_wait returned %d (%d ns elapsed)\n", ret, (unsigned) (t2 - t));

	return (void *) 35;
}


int init_module(void)
{
	int ret;
	hrtime_t t;
	int sleep = 500000000;
	pthread_attr_t attr;

	rtl_printf("RTLinux condvar test starts on CPU%d\n", rtl_getcpuid());
	pthread_attr_init (&attr);
	/* try to run the thread on another CPU */
	if (rtl_cpu_exists(!rtl_getcpuid())) {
		pthread_attr_setcpu_np(&attr, !rtl_getcpuid());
	}

	pthread_mutex_init (&mutex, NULL);
	pthread_cond_init (&cond, NULL);

	ret = pthread_create (&thread, &attr, start_routine, 0);
	if (ret) {
		rtl_printf("failed to create a thread\n");
		return ret;
	}

	rtl_printf("Linux thread is about to busy-wait for %d ns\n", sleep);
	t = gethrtime();
	while (gethrtime() < t + sleep);

	rtl_printf("Linux thread is about to signal the condition\n");
	pthread_mutex_lock (&mutex);
	ret = pthread_cond_signal (&cond);
	pthread_mutex_unlock (&mutex);
	rtl_printf("Linux thread: pthread_cond_signal returned %d\n", ret);

	return 0;
}


void cleanup_module(void)
{
	void *retval;
	pthread_cancel (thread);
	pthread_join (thread, &retval);
/* 	rtl_printf("RTLinux mutex: joined thread returned %d\n", (int) retval); */
	pthread_mutex_destroy (&mutex);
}
