/*
 * (C) Finite State Machine Labs Inc. 1997 business@fsmlabs.com
 *
 * Released under the terms of GPL 2.
 * Open RTLinux makes use of a patented process described in
 * US Patent 5,995,745. Use of this process is governed
 * by the Open RTLinux Patent License which can be obtained from
 * www.fsmlabs.com/PATENT or by sending email to
 * licensequestions@fsmlabs.com
 */

#include <rtl.h>
#include <rtl_fifo.h>
#include <time.h>
#include <rtl_sched.h>
#include <rtl_sync.h>
#include <pthread.h>
#include <unistd.h>
#include <rtl_debug.h>
#include <errno.h>
#include "rt_process.h"

int ntests = 500;
int period = 1000000;
int bperiod = 3100000;
int mode = 0;
int absolute = 0;
int fifo_size = 4000;
int advance = 0;

MODULE_PARM(period, "i");
MODULE_PARM(bperiod, "i");
MODULE_PARM(ntests, "i");
MODULE_PARM(mode, "i");
MODULE_PARM(absolute, "i");
MODULE_PARM(advance, "i");

pthread_t thread;
int fd_fifo;


void *thread_code(void *param)
{
	hrtime_t expected;
	hrtime_t diff;
	hrtime_t now;
	hrtime_t last_time = 0;
	hrtime_t min_diff;
	hrtime_t max_diff;
	struct sample samp;
	int i;
	int cnt = 0;

	if (mode) {
		int ret = rtl_setclockmode(CLOCK_REALTIME,
					   RTL_CLOCK_MODE_PERIODIC,
					   period);

		if (ret != 0) {
			conpr("Setting periodic mode failed\n");
			mode = 0;
		}
	} else {

		rtl_setclockmode(CLOCK_REALTIME, RTL_CLOCK_MODE_ONESHOT,
				 0);
	}

	expected =
	    clock_gethrtime(CLOCK_REALTIME) + 2 * (hrtime_t) period;

	if (advance) {
		pthread_make_periodic_np(pthread_self(),
					 expected - advance, period);
	} else {
		pthread_make_periodic_np(pthread_self(), expected, period);
	}


	fd_fifo = open("/dev/rtf0", O_NONBLOCK);
	if (fd_fifo < 0) {
		rtl_printf("/dev/rtf0 open returned %d\n", fd_fifo);
		return (void *) -1;
	}

	if (advance) {
		rtl_stop_interrupts();	/* Be careful with this! The task won't be preempted by anything else. This is probably only appropriate for small high-priority tasks. */
	}

	/* first cycle */
	pthread_wait_np();
	expected += period;
	now = clock_gethrtime(CLOCK_MONOTONIC);
	last_time = now;

	do {
		min_diff = 2000000000;
		max_diff = -2000000000;

		for (i = 0; i < ntests; i++) {
			++cnt;
			pthread_wait_np();

			now = clock_gethrtime(CLOCK_MONOTONIC);
			if (absolute && advance && !mode) {
				if (now < expected) {
					rtl_delay(expected - now);
				}
				now = clock_gethrtime(CLOCK_MONOTONIC);
			}
			if (absolute) {
				diff = now - expected;
			} else {
				diff = now - last_time - period;
				if (diff < 0) {
					diff = -diff;
				}
			}
			if (diff < min_diff) {
				min_diff = diff;
			}
			if (diff > max_diff) {
				max_diff = diff;
			}

			expected += period;
			last_time = now;
		}

		samp.min = min_diff;
		samp.max = max_diff;
		write(fd_fifo, &samp, sizeof(samp));
	} while (1);
	return 0;
}

pthread_t background_threadid;

void *background_thread(void *param)
{
	pthread_make_periodic_np(pthread_self(), gethrtime(), bperiod);
	while (1) {
		hrtime_t t = gethrtime();
		/* the measurement task should preempt the following loop */
		while (gethrtime() < t + bperiod * 2 / 3);
		pthread_wait_np();
	}
}


int init_module(void)
{
	pthread_attr_t attr;
	struct sched_param sched_param;
	int thread_status;
	int fifo_status;

	rtf_destroy(0);
	fifo_status = rtf_create(0, fifo_size);
	if (fifo_status) {
		rtl_printf
		    ("RTLinux measurement test fail. fifo_status=%d\n",
		     fifo_status);
		return -1;
	}


	pthread_attr_init(&attr);
	if (rtl_cpu_exists(1)) {
		pthread_attr_setcpu_np(&attr, 1);
	}
	sched_param.sched_priority = 1;
	pthread_attr_setschedparam(&attr, &sched_param);
	thread_status =
	    pthread_create(&thread, &attr, thread_code, (void *) 1);
	if (thread_status != 0) {
		rtl_printf("failed to create RT-thread: %d\n",
			   thread_status);
		return -1;
	}

	if (bperiod) {
		pthread_create(&background_threadid, NULL,
			       background_thread, NULL);
	}
	return 0;
}


void cleanup_module(void)
{
	pthread_cancel(thread);
	pthread_join(thread, NULL);
	close(fd_fifo);
	rtf_destroy(0);
	if (bperiod) {
		pthread_cancel(background_threadid);
		pthread_join(background_threadid, NULL);
	}
}
