/*
 * RTLinux semaphore implementation
 *
 * Written by Michael Barabanov <baraban@fsmlabs.com>
 * Copyright (C) Finite State Machine Labs Inc., 2000
 * Released under the terms of the GPL Version 2
 *
 */


#include <rtl_sched.h>
#include <rtl_sema.h>
#include <rtl_sync.h>
#include <errno.h>

int sem_init(sem_t *sem, int pshared, unsigned int value)
{
	rtl_wait_init (&sem->wait);
	sem->value = value;
	rtl_spin_lock_init(&sem->lock);
	return 0;
}

int sem_destroy(sem_t *sem)
{
	return 0;
}

int sem_getvalue(sem_t *sem, int *sval)
{
	*sval = sem->value;
	return 0;
}

int __sem_trywait(sem_t *sem)
{
	if (sem->value > 0) {
		--(sem->value);
		return 0;
	} else {
		errno = EAGAIN;
		return -1;
	}
}

int sem_trywait(sem_t *sem)
{
	rtl_irqstate_t flags;
	int ret;
	rtl_spin_lock_irqsave (&sem->lock, flags);
	ret = __sem_trywait(sem);
	rtl_spin_unlock_irqrestore(&sem->lock, flags);
	return ret;
}

int sem_wait(sem_t *sem)
{
	int ret;
	rtl_irqstate_t flags;
	rtl_spin_lock_irqsave (&sem->lock, flags);
	while (__sem_trywait(sem)) {
		if (errno != EAGAIN) {
			rtl_spin_unlock_irqrestore(&sem->lock, flags);
			return -1;
		}
#ifdef CONFIG_OC_PSIGNALS
		set_bit(RTL_THREAD_SIGNAL_INTERRUMPIBLE,&(pthread_self()->threadflags));
#endif
		ret = rtl_wait_sleep (&sem->wait, &sem->lock);
		pthread_testcancel();
		rtl_spin_lock(&sem->lock);
		
#ifdef CONFIG_OC_PSIGNALS		
	/* 
	   Per POSIX standard sem_wait can be interrumped by a signal.
	   To check if sem_wait exits by a user signal or by sem_post,
	   what is done is to check the bit RTL_THREAD_SIGNAL_INTERRUMPIBLE.
	   If it is set implies that no signal has interrumped us. 
	   So do_user_signal cleans it (if set) and puts the thread on ready state,
           after executing the signal handler. 
	*/
		if (!test_and_clear_bit(RTL_THREAD_SIGNAL_INTERRUMPIBLE,&(pthread_self()->threadflags))){
		  errno=EINTR;
		  rtl_spin_unlock_irqrestore(&sem->lock, flags);
		  return -1;
		}
#endif
	}

	rtl_spin_unlock_irqrestore(&sem->lock, flags);
	return 0;
}


#ifdef _RTL_POSIX_TIMEOUTS
int sem_timedwait(sem_t *sem,  const struct timespec *abstime)
{
	rtl_sigset_t ret;
	rtl_irqstate_t flags;
	hrtime_t save_resume_time;
	pthread_t self = pthread_self();
	hrtime_t timeout;

	if (abstime == (const struct timespec *) &self->timeval) {
		timeout = self->timeval;
	} else {
		timeout = timespec_to_ns(abstime);
	}
	timeout = __rtl_fix_timeout_for_clock(CLOCK_REALTIME, timeout);

	rtl_spin_lock_irqsave (&sem->lock, flags);
	while (__sem_trywait(sem)) {
		if (errno != EAGAIN) {
			rtl_spin_unlock_irqrestore(&sem->lock, flags);
			return -1;
		}

		save_resume_time = self->resume_time;
		__rtl_setup_timeout(self, timeout);

#ifdef CONFIG_OC_PSIGNALS
		set_bit(RTL_THREAD_SIGNAL_INTERRUMPIBLE,&self->threadflags);
#endif
		ret = rtl_wait_sleep (&sem->wait, &sem->lock);
		pthread_testcancel();
		rtl_spin_lock(&sem->lock);

		self->resume_time = save_resume_time;

		if (RTL_TIMED_OUT(&ret)) {
			errno = ETIMEDOUT;
			rtl_spin_unlock_irqrestore(&sem->lock, flags);
			return -1;
		}

#ifdef CONFIG_OC_PSIGNALS
	if (!test_and_clear_bit(RTL_THREAD_SIGNAL_INTERRUMPIBLE,&self->threadflags)){
#else
		if (RTL_SIGINTR(&ret)) {
#endif
			errno = EINTR;
			rtl_spin_unlock_irqrestore(&sem->lock, flags);
			return -1;
		}

	}

	rtl_spin_unlock_irqrestore(&sem->lock, flags);
	return 0;
}
#endif


int sem_post(sem_t *sem)
{
	rtl_irqstate_t flags;
	rtl_spin_lock_irqsave (&sem->lock, flags);

	++(sem->value);
	rtl_wait_wakeup (&sem->wait);

	rtl_spin_unlock_irqrestore(&sem->lock, flags);
	rtl_schedule(); /* make it fast! */
	return 0;
}

