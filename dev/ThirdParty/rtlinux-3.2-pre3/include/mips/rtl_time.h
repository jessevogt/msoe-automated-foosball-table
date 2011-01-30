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

#ifndef __RTL_ARCH_TIME_H__
#define __RTL_ARCH_TIME_H__

extern unsigned long loops_per_sec;

#include <asm/types.h>

typedef __s64 hrtime_t;		/* high-resolution time type (signed 64-bit) */

static inline hrtime_t timespec_to_ns(const struct timespec *ts)
{
	hrtime_t t;

	t = (ts->tv_sec * NSECS_PER_SEC) + ts->tv_nsec;
	return t;
}

static inline struct timespec timespec_from_ns(hrtime_t t)
{
	struct timespec ts;
	ts.tv_sec = t / NSECS_PER_SEC;
	ts.tv_nsec = t % NSECS_PER_SEC;
	return ts;
}

#define rdtscl(x)	x=read_32bit_cp0_register(CP0_COUNT)

extern hrtime_t gethrtime(void);	/* time in nanoseconds since bootup */
extern hrtime_t gethrtimeres(void);	/* resolution of gethrtime() in ns */

struct rtl_clock_arch {
	int istimerset;
	hrtime_t time;
};

#define RTL_CLOCK_ARCH_INITIALIZER { 0, 0 }
/* __delay originally from include/asm-mips/delay.h 
 * 	-Nathan <npsimons@fsmlabs.com> */
extern __inline__ void __delay(unsigned long loops)
{
	__asm__ __volatile__(".set\tnoreorder\n"
			     "1:\tbnez\t%0,1b\n\t"
			     "subu\t%0,1\n\t"
			     ".set\treorder":"=r"(loops):"0"(loops));
}

static inline void rtl_delay(long nanoseconds)
{
}
#endif
