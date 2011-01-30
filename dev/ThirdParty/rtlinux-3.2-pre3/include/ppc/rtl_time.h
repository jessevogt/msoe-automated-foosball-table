/*
 * Copyright (C) 1999 FSM Labs (http://www.fsmlabs.com/)
 *  Written by Cort Dougan <cort@fsmlabs.com>
 *
 */
#ifndef __RTL_ARCH_TIME_H__
#define __RTL_ARCH_TIME_H__

typedef long long hrtime_t; /* high-resolution time type (signed 64-bit) */

static inline hrtime_t timespec_to_ns (const struct timespec *ts)
{
	hrtime_t t;

	t = ((hrtime_t)ts->tv_sec * (hrtime_t)NSECS_PER_SEC) +
		(hrtime_t)ts->tv_nsec;
	return t;
}

static inline struct timespec timespec_from_ns (hrtime_t t)
{
  	struct timespec ts;
	/* We loose the upper 32-bits here -- Cort */
	ts.tv_sec = (unsigned long)t/NSECS_PER_SEC;
	ts.tv_nsec = (unsigned long)(t-(hrtime_t)ts.tv_sec *
				     (hrtime_t)NSECS_PER_SEC);
	return ts;
}

#define rdtscl(x) __asm__ __volatile__ ("mftb %0": "=r" (x))

extern hrtime_t gethrtime(void); /* time in nanoseconds since bootup */
extern hrtime_t gethrtimeres(void); /* resolution of gethrtime() in ns */

#define RTL_CLOCK_ARCH_INITIALIZER { 0 }
struct rtl_clock_arch {
	int istimerset;
};

extern __inline__ void __delay(unsigned int loops)
{
	if (loops != 0)
		__asm__ __volatile__("mtctr %0; 1: bdnz 1b" : :
				     "r" (loops) : "ctr");
}

static inline void rtl_delay(long nanoseconds)
{
	unsigned long loops;
	extern unsigned long loops_per_sec;
	
	/* compute (usecs * 2^32 / 10^6) * loops_per_sec / 2^32 */
	nanoseconds *= 0x10c6;		/* 2^32 / 10^6 */
	__asm__("mulhwu %0,%1,%2" : "=r" (loops) :
		"r" (nanoseconds), "r" (loops_per_sec));
	__delay(loops/1000);
}
#endif
