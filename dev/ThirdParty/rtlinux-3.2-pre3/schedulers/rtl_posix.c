/*
 * RTLinux POSIX support
 *
 * Written by Michael Barabanov
 * Copyright (C) Finite State Machine Labs Inc., 1999,2000
 * Released under the terms of the GPL Version 2
 *
 */

#include <rtl_sched.h>
#include <rtl_posix.h>


void rtl_posix_init (pthread_t th)
{
	struct rtl_posix_thread_struct *posixdata = RTL_POSIX_DATA(th);
	pthread_spin_init (&posixdata->exitlock, 0);
	posixdata->joining_thread = 0;
	posixdata->joined_thread_retval = 0;
}


void rtl_posix_cleanup(void *retval)
{
	pthread_t self = pthread_self();
	struct rtl_posix_thread_struct *posixdata = RTL_POSIX_DATA(self);

	pthread_spin_lock (&posixdata->exitlock);
	posixdata->retval = retval;
	while (test_bit (RTL_THREAD_JOINABLE, &self->threadflags) && !posixdata->joining_thread) {
		set_bit (RTL_THREAD_WAIT_FOR_JOIN, &self->threadflags);
		RTL_MARK_SUSPENDED (self);
		pthread_spin_unlock (&posixdata->exitlock);
		rtl_schedule();
		pthread_spin_lock (&posixdata->exitlock);
	}

	pthread_spin_unlock (&posixdata->exitlock);

}


void rtl_posix_on_delete(pthread_t th)
{
	struct rtl_posix_thread_struct *posixdata = RTL_POSIX_DATA(th);

	if (posixdata->joining_thread) {
		struct rtl_thread_struct *joining_thread = posixdata->joining_thread;
		struct rtl_posix_thread_struct *joindata = RTL_POSIX_DATA(joining_thread);
		pthread_spin_lock (&joindata->exitlock);
		joindata -> joined_thread_retval = posixdata -> retval;
		set_bit (RTL_THREAD_OK_TO_FINISH_JOIN, &joining_thread->threadflags);
		pthread_kill (joining_thread, RTL_SIGNAL_WAKEUP);
		pthread_spin_unlock (&joindata->exitlock);
	}
}


int pthread_join(pthread_t th, void **thread_return)
{
	pthread_t self = pthread_self();

	if (pthread_kill(th, 0)) {
		return ESRCH;
	}

	if (self == th){
	       	return EDEADLK;
	}

	pthread_spin_lock (&RTL_POSIX_DATA(th)->exitlock);
	if (!test_bit (RTL_THREAD_JOINABLE, &th->threadflags)) {
		pthread_spin_unlock (&RTL_POSIX_DATA(th)->exitlock);
		return EINVAL;
	}

	RTL_POSIX_DATA(th)->joining_thread = pthread_self();
	if (test_and_clear_bit(RTL_THREAD_WAIT_FOR_JOIN, &th->threadflags)) {
		pthread_kill(th, RTL_SIGNAL_WAKEUP);
	}
	pthread_spin_unlock (&RTL_POSIX_DATA(th)->exitlock);

	pthread_spin_lock (&RTL_POSIX_DATA(self)->exitlock);
	while (!test_and_clear_bit (RTL_THREAD_OK_TO_FINISH_JOIN, &self->threadflags)) {
		if (self != pthread_linux()) {
			RTL_MARK_SUSPENDED (self);
		} else {
			current->state = TASK_UNINTERRUPTIBLE;
		}

		pthread_spin_unlock (&RTL_POSIX_DATA(self)->exitlock);

		if (self != pthread_linux()) {
			rtl_schedule();
		} else {
			schedule_timeout(HZ/5);
		}

		pthread_spin_lock (&RTL_POSIX_DATA(self)->exitlock);
	}

	pthread_spin_unlock (&RTL_POSIX_DATA(self)->exitlock);

	if (thread_return) {
		*thread_return = RTL_POSIX_DATA(self)->joined_thread_retval;
	}

	return 0;
}


int pthread_detach(pthread_t thread)
{
	pthread_spin_lock (&RTL_POSIX_DATA(thread)->exitlock);
	if (!test_bit(RTL_THREAD_JOINABLE, &thread->threadflags)) {
		pthread_spin_unlock (&RTL_POSIX_DATA(thread)->exitlock);
		return EINVAL;
	}
	clear_bit (RTL_THREAD_JOINABLE, &thread->threadflags);
	if (test_and_clear_bit(RTL_THREAD_WAIT_FOR_JOIN, &thread->threadflags)) {
		pthread_kill(thread, RTL_SIGNAL_WAKEUP);
	}
	pthread_spin_unlock (&RTL_POSIX_DATA(thread)->exitlock);
	return 0;
}

