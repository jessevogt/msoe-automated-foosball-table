/*
 * RTLinux mutex and condvar implementation
 *
 * Written by Michael Barabanov
 * Rewritten by Victor Yodaiken after a long delay.
 * Copyright (C) Finite State Machine Labs Inc., 1999,2000
 * Released under the terms of the GPL Version 2
 *
 */
/* ChangeLog
 * Jan 5 2004 Der Herr Hofrat <der.herr@hofr.at>
 *        fix rtl_wait_wakeup on SMP - if thread is on other CPU make
 *        sure that scheduler is called.
 */

#include <rtl_conf.h>
#include <rtl_sched.h>
#include <rtl_mutex.h>
#include <rtl_printf.h>

#include <rtl_sync.h>

static void rtl_wait_abort(void *data)
{
	pthread_t self = pthread_self();
	struct rtl_wait_struct *t;
	rtl_wait_t *wait = (rtl_wait_t*) data;
	rtl_irqstate_t flags;

	rtl_spin_lock_irqsave (wait->p_lock, flags);
	if (!wait->queue) {
		goto done;
	}

	if (wait->queue->waiter == self) {
		wait->queue = wait->queue->next;
		goto done;
	}
	for (t = wait->queue; t->next; t = t->next) {
#ifdef CONFIG_RTL_WAITQUEUE_DEBUG
		if (t->magic != RTL_WAIT_MAGIC) {
			BUG();
		}
#endif
		if (t->next->waiter == self) {
			t->next = t->next->next;
			break;
		}
	}
done:	self->abort = 0;
	self->abortdata = 0;
	rtl_spin_unlock_irqrestore(wait->p_lock, flags);
}

int rtl_wait_sleep (rtl_wait_t *wait, spinlock_t *lock)
{
	pthread_t self = pthread_self();
	struct rtl_wait_struct wait_struct;

	self->abort = &rtl_wait_abort;
	self->abortdata = wait;

#ifdef CONFIG_RTL_WAITQUEUE_DEBUG
	wait_struct.magic = RTL_WAIT_MAGIC;
#endif
	wait_struct.waiter = self;
	wait_struct.next = wait->queue;
	wait->queue = &wait_struct;
	wait->p_lock = lock;

	RTL_MARK_SUSPENDED (self);

	rtl_spin_unlock(lock);

	return rtl_schedule();
}


int rtl_wait_wakeup (rtl_wait_t *wait)
{
	struct rtl_wait_struct *t;
	for (t = wait->queue; t; t = t->next) {
#ifdef CONFIG_RTL_WAITQUEUE_DEBUG
		if (t->magic != RTL_WAIT_MAGIC) {
			BUG();
		}
#endif
#ifdef CONFIG_SMP
		/* schedule the thread on the other cpu so that it is not
		 * delayed until the next call to schedule on that CPU !
		 * 
		 * The method here is implicidly resulting in a FIFO 
		 * order if the threads are on a non-current CPU which
		 * is not recomended by POSIX.
		 * TODO: mark all threads with pthread_kill, and call the
		 * scheduler only for the last one woken on the non-local
		 * CPU.
		 */		 
		if ((t->waiter)->cpu != rtl_getcpuid()) {
			pthread_kill(t->waiter, RTL_SIGNAL_WAKEUP);
//			rtl_reschedule_thread(t->waiter);
			rtl_reschedule((t->waiter)->cpu);
		} else
#endif
		{
			pthread_kill(t->waiter, RTL_SIGNAL_WAKEUP);
        	}
	}
	wait->queue = NULL;
	return 0;
}

int pthread_mutexattr_init(pthread_mutexattr_t *attr)
{
	attr->type = PTHREAD_MUTEX_DEFAULT;
	attr->pshared = PTHREAD_PROCESS_SHARED;
	attr->protocol = PTHREAD_PRIO_NONE;
	attr->prioceiling = sched_get_priority_max(SCHED_FIFO);
	return 0;
}

int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
      mutex->valid = 0;
      return 0;
}

int pthread_mutex_init(pthread_mutex_t *mutex,
		    const pthread_mutexattr_t *attr)
{
	pthread_mutexattr_t defattr;
        mutex->valid = 1;
	mutex->busy = 0;
	mutex->flags = 0;
	rtl_wait_init(&mutex->wait);
	rtl_spin_lock_init (&mutex->lock);
	if (!attr) {
		pthread_mutexattr_init(&defattr);
		attr = &defattr;
	}
	mutex->type = attr->type;
	mutex->protocol = attr->protocol;
	mutex->prioceiling = attr->prioceiling;
	return 0;
}


static inline int __pthread_mutex_trylock(pthread_mutex_t *mutex)
{
#ifdef _RTL_POSIX_THREAD_PRIO_PROTECT
/* if _RTL_POSIX_THREAD_PRIO_PROTECT, this code is protected by a spinlock */
	if (mutex->protocol == PTHREAD_PRIO_PROTECT) {
		if (RTL_PRIO(RTL_CURRENT) > mutex->prioceiling) {
			return EINVAL;
		}
		
		mutex->oldprio = RTL_PRIO (RTL_CURRENT);
		RTL_PRIO (RTL_CURRENT) = mutex->prioceiling;
	}
#endif
	if (test_and_set_bit(0, &mutex->busy)) {
		return EBUSY;
	}
	return 0;
}

int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
	rtl_irqstate_t flags;
	int ret;
	switch (mutex->type) {
		case PTHREAD_MUTEX_SPINLOCK_NP:
			rtl_no_interrupts (flags);
			if (rtl_spin_trylock(&mutex->lock)) {
				rtl_restore_interrupts (flags);
				return EBUSY;
			} else {
				mutex->flags = flags;
				return 0;
			}
			break;
		case PTHREAD_MUTEX_NORMAL:
		default:
#ifdef _RTL_POSIX_THREAD_PRIO_PROTECT
			rtl_spin_lock_irqsave (&mutex->lock, flags);
#endif
			ret = __pthread_mutex_trylock(mutex);
#ifdef _RTL_POSIX_THREAD_PRIO_PROTECT
			rtl_spin_unlock_irqrestore(&mutex->lock, flags);
#endif
			return ret;
	}
}


int pthread_mutex_lock(pthread_mutex_t *mutex)
{
	rtl_irqstate_t flags;
	int ret;

	switch (mutex->type) {
		case PTHREAD_MUTEX_SPINLOCK_NP:
			rtl_no_interrupts (flags);
			rtl_spin_lock (&mutex->lock);
			mutex->flags = flags;
			return 0;
		case PTHREAD_MUTEX_NORMAL:
		default:
			if (!mutex->valid) {
				return EINVAL;
			}

			rtl_spin_lock_irqsave (&mutex->lock, flags);
			while ((ret = __pthread_mutex_trylock(mutex))) {
				if (ret == EINVAL) {
					rtl_spin_unlock_irqrestore(&mutex->lock, flags);
					return ret;
				}
				
				ret = rtl_wait_sleep (&mutex->wait, &mutex->lock);
				rtl_spin_lock(&mutex->lock);

			}
			rtl_spin_unlock_irqrestore(&mutex->lock, flags);
			return 0;
	}
}


int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
	rtl_irqstate_t flags;

	switch (mutex->type) {
		case PTHREAD_MUTEX_SPINLOCK_NP:
			flags = mutex->flags;
			rtl_spin_unlock(&mutex->lock);
			rtl_restore_interrupts (flags);
			return 0;
		case PTHREAD_MUTEX_NORMAL:
		default:
			if (!mutex->valid) {
				return EINVAL;
			}

			rtl_spin_lock_irqsave (&mutex->lock, flags);
#ifdef _RTL_POSIX_THREAD_PRIO_PROTECT
			if (mutex->protocol == PTHREAD_PRIO_PROTECT) {
				RTL_PRIO(RTL_CURRENT) = mutex->oldprio;
			}
#endif
			clear_bit (0, &mutex->busy);
			rtl_wait_wakeup(&mutex->wait);
			rtl_spin_unlock_irqrestore (&mutex->lock, flags);
			/* XXX we do not call the scheduler here by design;
			 * for fast wakeups, use semaphores & pthread_wakeup_np */
			return 0;
	}
}

#ifdef _RTL_POSIX_TIMEOUTS
/* not supported for spinlock mutexes */
int pthread_mutex_timedlock(pthread_mutex_t *mutex, const struct timespec *abstime)
{
	rtl_sigset_t reason;
	int ret;
	rtl_irqstate_t flags;
	pthread_t self = pthread_self();
	hrtime_t save_resume_time;
	hrtime_t timeout;

	if (mutex->type == PTHREAD_MUTEX_SPINLOCK_NP) {
		return EINVAL;
	}
	if (!mutex->valid) {
		return EINVAL;
	}

	rtl_spin_lock_irqsave (&mutex->lock, flags);
	while ((ret = __pthread_mutex_trylock(mutex))) {
		if (ret == EINVAL) {
			rtl_spin_unlock_irqrestore(&mutex->lock, flags);
			return ret;
		}

		if (abstime == (const struct timespec *) &self->timeval) {
			timeout = self->timeval;
		} else {
			timeout = timespec_to_ns(abstime);
		}
		timeout = __rtl_fix_timeout_for_clock(CLOCK_REALTIME, timeout);

		save_resume_time = self->resume_time;
		__rtl_setup_timeout(self, timeout);

		reason = rtl_wait_sleep (&mutex->wait, &mutex->lock);
		if (RTL_TIMED_OUT(&reason)) {
			rtl_restore_interrupts(flags);
			return ETIMEDOUT;
		}
		rtl_spin_lock(&mutex->lock);

		self->resume_time = save_resume_time;

	}
	rtl_spin_unlock_irqrestore(&mutex->lock, flags);
	return 0;
}
#endif


#ifdef _RTL_POSIX_THREAD_PRIO_PROTECT
int pthread_mutex_setprioceiling(pthread_mutex_t *mutex, int prioceiling, int *old_ceiling)

{
	int ret;
	ret = pthread_mutex_lock (mutex);
	if (ret) {
		return ret;
	}
	*old_ceiling = mutex->prioceiling;
	mutex->prioceiling = prioceiling;
	pthread_mutex_unlock (mutex);
	return 0;
}
#endif


/* For condvar wait and signal operations the mutex should
 * be locked; otherwise the behaviour is undefined */

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
	int ret;
	rtl_irqstate_t flags;

	rtl_no_interrupts (flags);
	pthread_mutex_unlock (mutex);
	rtl_spin_lock(&cond->lock);
	ret = rtl_wait_sleep(&cond->wait, &cond->lock);
	pthread_mutex_lock (mutex);
	pthread_testcancel();
	rtl_restore_interrupts (flags);
	return 0;
}

int pthread_cond_broadcast(pthread_cond_t *cond)
{
	rtl_irqstate_t flags;
	rtl_spin_lock_irqsave (&cond->lock, flags);
	rtl_wait_wakeup(&cond->wait);
	rtl_spin_unlock_irqrestore (&cond->lock, flags);
	return 0;
}


int pthread_cond_timedwait(pthread_cond_t *cond,
		    pthread_mutex_t *mutex, const struct timespec *abstime)
{
	rtl_sigset_t reason;
	rtl_irqstate_t flags;
	pthread_t self = pthread_self();
	hrtime_t save_resume_time;
	hrtime_t timeout;

	if (abstime == (const struct timespec *) &self->timeval) {
		timeout = self->timeval;
	} else {
		timeout = timespec_to_ns(abstime);
	}
	timeout = __rtl_fix_timeout_for_clock(CLOCK_REALTIME, timeout);

	rtl_no_interrupts (flags);
	pthread_mutex_unlock (mutex);

	save_resume_time = self->resume_time;
	__rtl_setup_timeout(self, timeout);

	rtl_spin_lock(&cond->lock);
	reason = rtl_wait_sleep(&cond->wait, &cond->lock);
	self->resume_time = save_resume_time;

	pthread_mutex_lock (mutex);
	pthread_testcancel();

	rtl_restore_interrupts (flags);
	return RTL_TIMED_OUT(&reason) ? ETIMEDOUT : 0;
}

int pthread_cond_init(pthread_cond_t *cond,
		    const pthread_condattr_t *attr)
{
	rtl_wait_init(&cond->wait);
	spin_lock_init(&cond->lock);
	return 0;
}

int pthread_cond_destroy(pthread_cond_t *mutex)
{
	return 0;
}

