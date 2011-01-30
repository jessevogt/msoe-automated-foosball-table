/*
 * RTLinux mutex implementation
 *
 * Written by Michael Barabanov
 * Copyright (C) Finite State Machine Labs Inc., 1999
 * Released under the terms of the GPL Version 2
 *
 */

#ifndef __RTL_MUTEX__
#define __RTL_MUTEX__

#ifdef __KERNEL__

#include <rtl_conf.h>
#include <errno.h>
#include <rtl_spinlock.h>

enum { PTHREAD_MUTEX_NORMAL, PTHREAD_MUTEX_RECURSIVE, PTHREAD_MUTEX_ERRORCHECK,
	PTHREAD_MUTEX_SPINLOCK_NP, PTHREAD_MUTEX_DEFAULT = PTHREAD_MUTEX_NORMAL };

typedef struct {
	int type;
	int pshared;
	int protocol;
	int prioceiling;
} pthread_mutexattr_t;

#define PTHREAD_PROCESS_PRIVATE 0
#define PTHREAD_PROCESS_SHARED 1

#define RTL_WAIT_MAGIC 0x43743543

struct rtl_thread_struct;

struct rtl_wait_struct {
	int magic;
	struct rtl_thread_struct *waiter;
	struct rtl_wait_struct *next;
};

struct rtl_waitqueue_struct {
	struct rtl_wait_struct *queue;
	spinlock_t *p_lock;
};

typedef struct rtl_waitqueue_struct rtl_wait_t;

#define rtl_wait_init(q) do { (q)->queue = 0; /* spin_lock_init(&((q)->lock)); */ } while (0)
extern int rtl_wait_sleep (rtl_wait_t *wait, spinlock_t *lock);
extern int rtl_wait_wakeup (rtl_wait_t *wait);
#define RTL_WAIT_INITIALIZER { 0 /*, SPIN_LOCK_UNLOCKED */}

typedef struct {
	rtl_irqstate_t flags;
	int busy;
	int valid;
	spinlock_t lock;
	int type;
	rtl_wait_t wait;
	int protocol;
	int prioceiling;
	int oldprio;
} pthread_mutex_t;


#define PTHREAD_MUTEX_INITIALIZER { 0,0,1, SPIN_LOCK_UNLOCKED, PTHREAD_MUTEX_DEFAULT, RTL_WAIT_INITIALIZER, PTHREAD_PRIO_NONE }

enum {PTHREAD_PRIO_NONE, PTHREAD_PRIO_PROTECT};


static inline int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type)
{
	if (type != PTHREAD_MUTEX_NORMAL
			&& attr->type != PTHREAD_MUTEX_SPINLOCK_NP) {
		return EINVAL;
	}
	attr->type = type;
	return 0;
}



static inline int pthread_mutexattr_getpshared(const pthread_mutexattr_t *attr,
		    int *pshared)
{
	*pshared = attr->pshared;
	return 0;
}

/*TODO something sensible with this. We don't worry about
  shared and not shared yet.
  */
extern inline int pthread_mutexattr_setpshared(pthread_mutexattr_t *attr,
		    int pshared)
{
	if (pshared != PTHREAD_PROCESS_SHARED
			&& pshared != PTHREAD_PROCESS_PRIVATE) {
		return EINVAL;
	}
	attr->pshared = pshared;
	return 0;
}


extern int pthread_mutexattr_init(pthread_mutexattr_t *attr);

extern inline int pthread_mutexattr_destroy(pthread_mutexattr_t *attr)
{
	return 0;
}

extern int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type);

extern inline int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *type)
{
	*type = attr->type;
	return 0;
}


extern int pthread_mutex_init(pthread_mutex_t *mutex,
		    const pthread_mutexattr_t *attr);
extern inline int pthread_mutex_destroy(pthread_mutex_t *mutex);

extern int pthread_mutex_lock(pthread_mutex_t *mutex);
extern int pthread_mutex_trylock(pthread_mutex_t *mutex);
extern int pthread_mutex_unlock(pthread_mutex_t *mutex);

/* not supported for spinlock mutexes */
extern int pthread_mutex_timedlock(pthread_mutex_t *mutex, const struct timespec *abstime);

static inline int pthread_mutexattr_setprotocol(pthread_mutexattr_t *attr, int protocol)
{
#ifndef _RTL_POSIX_THREAD_PRIO_PROTECT
		return ENOTSUP;
#endif
	if (protocol != PTHREAD_PRIO_PROTECT && protocol != PTHREAD_PRIO_NONE) {
		return ENOTSUP;
	}
	attr->protocol = protocol;
	return 0;
}

static inline int pthread_mutexattr_getprotocol(const pthread_mutexattr_t *attr, int *protocol)
{
	*protocol = attr->protocol;
	return 0;
}

static inline int pthread_mutexattr_setprioceiling(pthread_mutexattr_t *attr, int prioceiling)
{
	attr->prioceiling = prioceiling;
	return 0;
}

static inline int pthread_mutexattr_getprioceiling(const pthread_mutexattr_t *attr, int *prioceiling)
{
	*prioceiling = attr->prioceiling;
	return 0;
}


extern int pthread_mutex_setprioceiling(pthread_mutex_t *mutex, int prioceiling, int *old_ceiling);

static inline int pthread_mutex_getprioceiling(const pthread_mutex_t *mutex, int *prioceiling)
{
	*prioceiling = mutex->prioceiling;
	return 0;
}




typedef struct {
	int pshared;
} pthread_condattr_t;

extern inline int pthread_condattr_getpshared(const pthread_condattr_t *attr,
		    int *pshared)
{
	*pshared = attr->pshared;
	return 0;
}

extern inline int pthread_condattr_setpshared(pthread_condattr_t *attr,
		    int pshared) {
	if (pshared != PTHREAD_PROCESS_SHARED
			&& pshared != PTHREAD_PROCESS_PRIVATE) {
		return EINVAL;
	}
	attr->pshared = pshared;
	return 0;
}


extern inline int pthread_condattr_init(pthread_condattr_t *attr)
{
	attr->pshared = PTHREAD_PROCESS_PRIVATE;
	return 0;
}

extern inline int pthread_condattr_destroy(pthread_condattr_t *attr)
{
	return 0;
}

typedef struct {
	rtl_wait_t wait;
	spinlock_t lock;
} pthread_cond_t;


#define PTHREAD_COND_INITIALIZER { RTL_WAIT_INITIALIZER, SPIN_LOCK_UNLOCKED }

extern int pthread_cond_init(pthread_cond_t *cond,
		    const pthread_condattr_t *attr);
extern int pthread_cond_destroy(pthread_cond_t *cond);

extern int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);

extern int pthread_cond_broadcast(pthread_cond_t *cond);
#define pthread_cond_signal(cond) pthread_cond_broadcast(cond)

extern int pthread_cond_timedwait(pthread_cond_t *cond,
		    pthread_mutex_t *mutex, const struct timespec *abstime);

#endif

#endif
