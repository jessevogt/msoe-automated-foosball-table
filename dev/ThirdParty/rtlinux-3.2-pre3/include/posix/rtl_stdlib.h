/*
 * RTLinux stdlib.h support
 *
 * Written by Michael Barabanov
 * Copyright (C) Finite State Machine Labs Inc., 2000
 * Released under the terms of the GPL Version 2
 *
 */

#ifndef __RTL_STDLIB_H__
#define __RTL_STDLIB_H__

#ifdef __KERNEL__

#include <pthread.h>
#include <linux/mm.h>

#ifdef __CPLUSPLUS__
extern "C" {
#endif
	void *kmalloc(unsigned size, int prio);
	void kfree(const void *p);
#ifdef __CPLUSPLUS__
};
#endif

static inline void *malloc(size_t size)
{
	void *ret;
	if (pthread_self() != pthread_linux() ||
			!(ret = kmalloc(size, GFP_KERNEL))) {
		errno = ENOMEM;
		return 0;
	}
	return ret;
}

static inline void free(void *ptr)
{
	if (pthread_self() != pthread_linux()) {
		return;
	}
	kfree(ptr);
}

#endif /* __KERNEL__ */
#endif
