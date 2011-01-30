/*
 * (C) Finite State Machine Labs Inc. 2000 business@fsmlabs.com
 *
 * Released under the terms of GPL 2.
 * Open RTLinux makes use of a patented process described in
 * US Patent 5,995,745. Use of this process is governed
 * by the Open RTLinux Patent License which can be obtained from
 * www.fsmlabs.com/PATENT or by sending email to
 * licensequestions@fsmlabs.com
 */

#include <rtl.h>
#include <pthread.h>
#include <rtl_sched.h>
#include <rtl_time.h>
#include <rtl_fifo.h>

#define MODULE_NAME	"thread_time"

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Nathan Paul Simons <npsimons@fsmlabs.com>");
MODULE_DESCRIPTION("RTLinux thread wait test kernel module");

MODULE_PARM(times, "i");
MODULE_PARM_DESC(times, "Number of times to test.");
MODULE_PARM(fifo, "i");
MODULE_PARM_DESC(fifo, "RTL-FIFO number to create (ie, 0 -> /dev/rtf0)");
MODULE_PARM(wait, "i");
MODULE_PARM_DESC(wait,
		 "Amount of time to wait (in nanoseconds) for each test");

int times;
int fifo = 0;
int wait;

int all_times_sz;

pthread_t time_thread;
pthread_t fifo_thread;

char *rtl_strerr(int thiserr);

void *time_stats(void *arg)
{
	struct sched_param my_sparam;
	int retval;
	int i;
	hrtime_t all_times[times];

	my_sparam.sched_priority = 1;

	if (
	    (retval =
	     pthread_setschedparam(pthread_self(), SCHED_FIFO,
				   &my_sparam)) != 0) {
		rtl_printf
		    ("%s: pthread_setschedparam (): %s\n", MODULE_NAME,
		     rtl_strerr(retval));
		return (NULL);
	}

	all_times[0] = clock_gethrtime(CLOCK_REALTIME);
	pthread_make_periodic_np(pthread_self(), all_times[0], wait);

	for (i = 0; i <= times; i++) {
		pthread_wait_np();
		all_times[i] = clock_gethrtime(CLOCK_REALTIME);
	}

	if ((retval = rtf_put(fifo, all_times, all_times_sz)) <
	    all_times_sz) {
		rtl_printf("%s: rtf_put (%d, all_times, %d): %s\n",
			   MODULE_NAME, fifo, all_times_sz,
			   rtl_strerr(retval));
		return (NULL);
	}

	return (NULL);
}

int init_module(void)
{
	int retval;

	fifo = 0;
	all_times_sz = (sizeof(hrtime_t) * (times + 1));

	if ((retval = rtf_create(fifo, all_times_sz + 1)) != 0) {
		rtl_printf("%s: rtf_create (0, %d): %s\n", MODULE_NAME,
			   all_times_sz, rtl_strerr(retval));
		return (retval);
	}

	if ((retval = pthread_create(&time_thread, NULL, time_stats, 0)) !=
	    0) {
		rtl_printf
		    ("%s: pthread_create (&time_thread, NULL, time_stats, 0): %s\n",
		     MODULE_NAME, rtl_strerr(retval));
		return (retval);
	}

	return (0);
}

void cleanup_module(void)
{
	pthread_cancel(time_thread);
	pthread_join(time_thread, NULL);

	rtf_destroy(fifo);
}

char *rtl_strerr(int thiserr)
{
	switch (thiserr) {
	case -ENODEV:
		return ("-ENODEV");
	case -EINVAL:
		return ("-EINVAL");
	case -EPERM:
		return ("-EPERM");
	case -ESRCH:
		return ("-ESRCH");
	case -EFAULT:
		return ("-EFAULT");
	case -EBUSY:
		return ("-EBUSY");
	case -ENOMEM:
		return ("-ENOMEM");
	case -ENOSPC:
		return ("-ENOSPC");
	case -EAGAIN:
		return ("-EAGAIN");
	default:
		return ("unknown error");
	}
}
