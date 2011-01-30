/*
 * Copyright (C) 2000 FSM Labs (http://www.fsmlabs.com/)
 *  Written by Cort Dougan <cort@fsmlabs.com>
 *
 */
#ifndef __RTL_ARCH_TIME_H__
#define __RTL_ARCH_TIME_H__

extern unsigned long loops_per_sec;

typedef long hrtime_t; /* high-resolution time type (signed 64-bit) */

static inline hrtime_t timespec_to_ns (const struct timespec *ts)
{
	hrtime_t t;

	t = (ts->tv_sec * NSECS_PER_SEC) + ts->tv_nsec;
	return t;
}

static inline struct timespec timespec_from_ns (hrtime_t t)
{
  	struct timespec ts;
	ts.tv_sec = t/NSECS_PER_SEC;
	ts.tv_nsec = t%NSECS_PER_SEC;
	return ts;
}

#define rdtscl(x) __asm__ __volatile__ ("rpcc %0" : "=r" (x))

extern hrtime_t gethrtime(void); /* time in nanoseconds since bootup */
extern hrtime_t gethrtimeres(void); /* resolution of gethrtime() in ns */

struct rtl_clock_arch {
	int istimerset;
	int count_irqs;
};

#define RTL_CLOCK_ARCH_INITIALIZER { 0, 0 }

extern __inline__ void __delay(unsigned int loops)
{
	int tmp;
	__asm__ __volatile__(
		"	rpcc %0\n"
		"	addl %1,%0,%1\n"
		"1:	rpcc %0\n"
		"	subl %1,%0,%0\n"
		"	bgt %0,1b"
		: "=&r" (tmp), "=r" (loops) : "1"(loops));
}


static inline void rtl_delay(long nanoseconds)
{
	nanoseconds *= ((1UL << 32) / 10000000) * loops_per_sec;
	__delay((long)nanoseconds >> 32);
}
#endif
