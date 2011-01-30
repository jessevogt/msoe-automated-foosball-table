/*
 * rtl_time.h
 *
 * RTLinux architecture-independent clock support
 *
 * Copyright (C) 1999 Michael Barabanov
 *
 * Fixed overflow in timespec_add_ns Josep Vidal (OCERA)
 * 
 */

#ifndef __RTL_TIME__
#define __RTL_TIME__



#define NSECS_PER_SEC 1000000000
#define HRTIME_INFINITY 0x7fffFfffFfffFfffLL

#ifdef __KERNEL__

#include <asm/ptrace.h>
#include <rtl_conf.h>

extern int *(*__errno_location_ptr)(void);
extern int *__errno_location(void);
#define errno (*__errno_location())

/* for timespec */
#include <linux/posix_types.h>
#include <linux/time.h>
#endif

#include <arch/rtl_time.h>

#define HRT_FROM_NS(x) (x)
#define HRTICKS_PER_SEC 1000000000

#ifndef HRT_FROM_8254
#define HRT_FROM_8254(x) ((x) * 838)
#endif

#ifndef HRT_TO_8254
#define HRT_TO_8254(x) ((x) / 838)
#endif

#ifdef __KERNEL__

#include <rtl_spinlock.h>

typedef void (* clock_irq_handler_t)(struct pt_regs *r);

enum { RTL_CLOCK_MODE_UNINITIALIZED = 1, RTL_CLOCK_MODE_ONESHOT,
RTL_CLOCK_MODE_PERIODIC};

struct rtl_clock {
	int (*init) (struct rtl_clock *);
	void (*uninit) (struct rtl_clock *);
	hrtime_t (*gethrtime)(struct rtl_clock *);
	int (*sethrtime)(struct rtl_clock *, hrtime_t t);

	int (*settimer)(struct rtl_clock *, hrtime_t interval);
	int (*settimermode)(struct rtl_clock *, int mode);
	clock_irq_handler_t handler;
	int mode;
	hrtime_t resolution;
	hrtime_t value;
	hrtime_t delta;
	pthread_spinlock_t lock;
	struct rtl_clock_arch arch;
};

typedef struct rtl_clock *clockid_t;

extern struct rtl_clock RTL_CLOCK_DEFAULTS;

extern int rtl_setclockhandler (clockid_t h, clock_irq_handler_t fn);
extern int rtl_unsetclockhandler (clockid_t h);

extern int rtl_clockadjust (clockid_t clock_id, hrtime_t delta);

/* scheduler interface */
static inline hrtime_t clock_gethrtime (clockid_t clock_id)
{
	return clock_id->gethrtime (clock_id);
}

extern clockid_t rtl_getbestclock (unsigned int cpu);


/* end of scheduler interface */


extern int rtl_init_standard_clocks(void);
extern void rtl_cleanup_standard_clocks(void);

#ifdef CONFIG_RTL_CLOCK_GPOS
extern clockid_t CLOCK_GPOS;
#endif

#endif /* __KERNEL__ */
/* convenience functions */
#define timespec_normalize(t) {\
        if ((t)->tv_nsec >= NSECS_PER_SEC) { \
                (t)->tv_nsec -= NSECS_PER_SEC; \
                (t)->tv_sec++; \
        } else if ((t)->tv_nsec < 0) { \
                (t)->tv_nsec += NSECS_PER_SEC; \
                (t)->tv_sec--; \
        } \
}

#define timespec_add(t1, t2) do { \
	(t1)->tv_nsec += (t2)->tv_nsec;  \
	(t1)->tv_sec += (t2)->tv_sec; \
	timespec_normalize(t1);\
} while (0)

#define timespec_sub(t1, t2) do { \
	(t1)->tv_nsec -= (t2)->tv_nsec;  \
	(t1)->tv_sec -= (t2)->tv_sec; \
	timespec_normalize(t1);\
} while (0)

#define TWOSECONDS (NSECS_PER_SEC*2)
#define timespec_add_ns(t,n) do { \
  long long aux=(t)->tv_nsec+(n);\
  \
  if ((aux > TWOSECONDS) || (aux < -TWOSECONDS)) /*check overflow*/ {\
    (t)->tv_nsec +=((n) % NSECS_PER_SEC) ; \
    (t)->tv_sec += ((n) / NSECS_PER_SEC); \
  } else {  (t)->tv_nsec=aux; }\
  \
  timespec_normalize(t); \
}  while (0)

#define timespec_nz(t) ((t)->tv_sec != 0 || (t)->tv_nsec != 0)

#define timespec_lt(t1, t2) ((t1)->tv_sec < (t2)->tv_sec || ((t1)->tv_sec == (t2)->tv_sec && (t1)->tv_nsec < (t2)->tv_nsec))

#define timespec_gt(t1, t2) (timespec_lt(t2, t1))

#define timespec_ge(t1, t2) (!timespec_lt(t1, t2))

#define timespec_le(t1, t2) (!timespec_gt(t1, t2))

#define timespec_eq(t1, t2) ((t1)->tv_sec == (t2)->tv_sec && (t1)->tv_nsec == (t2)->tv_nsec)

#endif

