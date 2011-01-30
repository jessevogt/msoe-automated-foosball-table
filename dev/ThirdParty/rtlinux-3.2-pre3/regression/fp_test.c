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

#define MODULE_NAME	"fp_test"

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Nathan Paul Simons <npsimons@fsmlabs.com>");
MODULE_DESCRIPTION("RTLinux floating point test kernel module");

MODULE_PARM(fifo_sz, "i");
MODULE_PARM_DESC(fifo_sz, "Size of FIFO.");
MODULE_PARM(fifo_nr, "i");
MODULE_PARM_DESC(fifo_nr,
		 "RTL-FIFO number to create (ie, 0 -> /dev/rtf0)");

int fifo_nr = 1, fifo_sz = 8192;

float marker1 = 0;
float marker2 = 0;
float marker3 = 0;
float marker4 = 0;

static pthread_t thread1;
static pthread_t thread2;
static pthread_mutex_t fifo_lock;

char *rtl_strerr(int thiserr)
{
	switch (thiserr) {
	case EINVAL:
		return "EINVAL";
	case EDEADLK:
		return "EDEADLK";
	case EPERM:
		return "EPERM";
	case -ENODEV:
		return "-ENODEV";
	case -EINVAL:
		return "-EINVAL";
	case -EPERM:
		return "-EPERM";
	case -ESRCH:
		return "-ESRCH";
	case -EFAULT:
		return "-EFAULT";
	case -EBUSY:
		return "-EBUSY";
	case -ENOMEM:
		return "-ENOMEM";
	case -ENOSPC:
		return "-ENOSPC";
	case -EAGAIN:
		return "-EAGAIN";
	default:
		return "unknown error";
	}
}

void *handler1(void *arg)
{
	int retval;
	struct sched_param my_sparam;
	pthread_setfp_np(pthread_self(), 1);

	my_sparam.sched_priority = 1;

	if (
	    (retval =
	     pthread_setschedparam(pthread_self(), SCHED_FIFO,
				   &my_sparam)) != 0) {
		rtl_printf("%s: pthread_setschedparam(): %s\n",
			   MODULE_NAME, rtl_strerr(retval));
		return NULL;
	}

	pthread_make_periodic_np(pthread_self(),
				 clock_gethrtime(CLOCK_REALTIME),
				 50000000);

	/* anybody recognize this value? ;) */
	marker1 = 3.1415926535897932384626433832795;
	marker3 = .25;

	pthread_wait_np();

	marker1 = marker3 * marker1;
	marker3 = marker1 / .6578;

	pthread_wait_np();

	marker1 = marker1 + 2.30;
	marker3 = marker3 - 2.30;

	if ((retval = pthread_mutex_lock(&fifo_lock)) != 0) {
		rtl_printf("%s: pthread_mutex_lock(&fifo_lock): %s\n",
			   MODULE_NAME, rtl_strerr(retval));
		return (void *) retval;
	}

	if ((retval = rtf_put(fifo_nr, "handler1:", 9)) < 9) {
		rtl_printf("%s: rtf_put(%d, \"handler1:\", 9): %s\n",
			   MODULE_NAME, fifo_nr, rtl_strerr(retval));
		return (void *) retval;
	}

	if ((retval = rtf_put(fifo_nr, &marker1, sizeof(marker1))) <
	    sizeof(marker1)) {
		rtl_printf("%s: rtf_put(%d, marker1, %d): %s\n",
			   MODULE_NAME, fifo_nr, sizeof(marker1),
			   rtl_strerr(retval));
		return (void *) retval;
	}

	if ((retval = rtf_put(fifo_nr, ":", 1)) < 1) {
		rtl_printf("%s: rtf_put(%d, \":\", 1): %s\n",
			   MODULE_NAME, fifo_nr, rtl_strerr(retval));
		return (void *) retval;
	}

	if ((retval = rtf_put(fifo_nr, &marker3, sizeof(marker3))) <
	    sizeof(marker3)) {
		rtl_printf("%s: rtf_put(%d, marker3, %d): %s\n",
			   MODULE_NAME, fifo_nr, sizeof(marker3),
			   rtl_strerr(retval));
		return (void *) retval;
	}

	if ((retval = rtf_put(fifo_nr, ":", 1)) < 1) {
		rtl_printf("%s: rtf_put(%d, \":\", 1): %s\n",
			   MODULE_NAME, fifo_nr, rtl_strerr(retval));
		return (void *) retval;
	}

	if ((retval = pthread_mutex_unlock(&fifo_lock)) != 0) {
		rtl_printf("%s: pthread_mutex_unlock(&fifo_lock): %s\n",
			   MODULE_NAME, rtl_strerr(retval));
		return (void *) retval;
	}

	return NULL;
}

void *handler2(void *arg)
{
	int retval;
	struct sched_param my_sparam;
	pthread_setfp_np(pthread_self(), 1);

	my_sparam.sched_priority = 2;

	if (
	    (retval =
	     pthread_setschedparam(pthread_self(), SCHED_FIFO,
				   &my_sparam)) != 0) {
		rtl_printf("%s: pthread_setschedparam(): %s\n",
			   MODULE_NAME, rtl_strerr(retval));
		return NULL;
	}

	pthread_make_periodic_np(pthread_self(),
				 clock_gethrtime(CLOCK_REALTIME),
				 50000000);

	/* this ones a little harder to recognize, but anyone with a grounding
	 * in probability should know it. */
	marker2 = 2.7182818284590452353602874713526;

	pthread_wait_np();

	marker4 = -.25;

	pthread_wait_np();

	marker2 = marker4 + .000001;

	pthread_wait_np();

	marker4 = marker4 - .001001;

	pthread_wait_np();

	marker2 = marker2 * .100001;

	pthread_wait_np();

	marker4 = marker2 / .101101;

	pthread_wait_np();

	if ((retval = pthread_mutex_lock(&fifo_lock)) != 0) {
		rtl_printf("%s: pthread_mutex_lock(&fifo_lock): %s\n",
			   MODULE_NAME, rtl_strerr(retval));
		return (void *) retval;
	}

	if ((retval = rtf_put(fifo_nr, "handler2:", 9)) < 9) {
		rtl_printf("%s: rtf_put(%d, \"handler2:\", 9): %s\n",
			   MODULE_NAME, fifo_nr, rtl_strerr(retval));
		return (void *) retval;
	}

	if ((retval = rtf_put(fifo_nr, &marker2, sizeof(marker2))) <
	    sizeof(marker2)) {
		rtl_printf("%s: rtf_put(%d, marker2, %d): %s\n",
			   MODULE_NAME, fifo_nr, sizeof(marker2),
			   rtl_strerr(retval));
		return (void *) retval;
	}

	if ((retval = rtf_put(fifo_nr, ":", 1)) < 1) {
		rtl_printf("%s: rtf_put(%d, \":\", 1): %s\n",
			   MODULE_NAME, fifo_nr, rtl_strerr(retval));
		return (void *) retval;
	}

	if ((retval = rtf_put(fifo_nr, &marker4, sizeof(marker4))) <
	    sizeof(marker4)) {
		rtl_printf("%s: rtf_put(%d, marker4, %d): %s\n",
			   MODULE_NAME, fifo_nr, sizeof(marker4),
			   rtl_strerr(retval));
		return (void *) retval;
	}

	if ((retval = rtf_put(fifo_nr, ":", 1)) < 1) {
		rtl_printf("%s: rtf_put(%d, \":\", 1): %s\n",
			   MODULE_NAME, fifo_nr, rtl_strerr(retval));
		return (void *) retval;
	}

	if ((retval = pthread_mutex_unlock(&fifo_lock)) != 0) {
		rtl_printf("%s: pthread_mutex_unlock(&fifo_lock): %s\n",
			   MODULE_NAME, rtl_strerr(retval));
		return (void *) retval;
	}

	return NULL;
}

int init_module(void)
{
	int retval;

	pthread_mutex_init(&fifo_lock, NULL);

	rtf_destroy(fifo_nr);

	if ((retval = rtf_create(fifo_nr, fifo_sz)) != 0) {
		rtl_printf("%s: rtf_create(%d, %d): %s\n", MODULE_NAME,
			   fifo_nr, fifo_sz, rtl_strerr(retval));
		return retval;
	}

	if ((retval = pthread_create(&thread1, NULL, handler1, 0))) {
		rtl_printf
		    ("%s: pthread_create(&thread1, NULL, handler1, 0): %s\n",
		     MODULE_NAME, rtl_strerr(retval));
		return retval;
	}

	if ((retval = pthread_create(&thread2, NULL, handler2, 0))) {
		rtl_printf
		    ("%s: pthread_create(&thread2, NULL, handler2, 0): %s\n",
		     MODULE_NAME, rtl_strerr(retval));
		return retval;
	}

	return 0;
}

void cleanup_module(void)
{
	pthread_cancel(thread1);
/*	pthread_join(thread1, NULL); */
	pthread_cancel(thread2);
/*	pthread_join(thread2, NULL); */
}
