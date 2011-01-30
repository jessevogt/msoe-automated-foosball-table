/*
 * RTLinux time support
 *
 * Written by Michael Barabanov
 * Copyright (C) Finite State Machine Labs Inc., 1999
 * Released under the terms of the GPL Version 2
 *
 */

#ifndef __RTL_TIME_H__
#define __RTL_TIME_H__

#include <rtl_sched.h>
#include <rtl_time.h>
#include <errno.h>

extern clockid_t CLOCK_UST; /* unadjusted system time */

#define CLOCK_MONOTONIC CLOCK_UST

/* we may need to move these to the scheduler so that it could take
 * some actions when clock is adjusted (expire timers etc) */

static inline int clock_gettime(clockid_t clock_id, struct timespec *tp)
{
	hrtime_t t = clock_id->gethrtime(clock_id);
	if (t == (hrtime_t) -1) {
		__set_errno(EINVAL);
		return -1;
	}
	*tp = timespec_from_ns (t);
	return 0;
}

static inline int clock_settime(clockid_t clock_id, const struct timespec *tp)
{
	int ret = clock_id->sethrtime (clock_id, timespec_to_ns (tp));
	if (ret < 0) {
		__set_errno(-ret);
		return -1;
	}
	return 0;
}

static inline int clock_getres(clockid_t clock_id, struct timespec *res)
{
	if (res != NULL) {
		*res = timespec_from_ns (clock_id->resolution);
	}
	return 0;
}

static inline time_t time(time_t *tloc)
{
	struct timespec ts;
	hrtime_t t = clock_gethrtime(CLOCK_REALTIME);
	ts = timespec_from_ns (t);
	if (tloc) {
		*tloc = ts.tv_sec;
	}
	return ts.tv_sec;
}

#ifdef CONFIG_OC_PTIMERS
#include <rtl_timer.h>
#endif

#endif
































































