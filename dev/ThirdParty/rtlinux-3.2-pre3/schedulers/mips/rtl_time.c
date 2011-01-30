/*
 * MIPS-specific clock support
 *
 * Copyright (C) 2000 FSM Labs (http://www.fsmlabs.com/)
 *  Written by Cort Dougan <cort@fsmlabs.com>
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/timex.h>

#include <asm/system.h>
#include <asm/io.h>
#include <asm/mipsregs.h>

#include <rtl_core.h>
#include <rtl_time.h>
#include <rtl_sync.h>
#include <rtl.h>

MODULE_AUTHOR("Cort Dougan <cort@fsmlabs.com>");
MODULE_DESCRIPTION("RTLinux MIPS Timer Module");

static void _orion_uninit (clockid_t clock);
static int _orion_init (clockid_t clock);
static hrtime_t _orion_gettime (struct rtl_clock *);
static int _orion_settimer (struct rtl_clock *, hrtime_t interval);
static int _orion_settimermode (struct rtl_clock *, int mode);
unsigned int orion_intercept(struct pt_regs *);
void default_handler( struct pt_regs *regs);

static unsigned long linux_decrs = 0, orion_tick_count = 0;
static unsigned int timerhi = 0, timerlo = 0;

struct rtl_clock orion_clock =
{
	_orion_init, _orion_uninit,
	_orion_gettime,
	NULL, /* sethrtime */
	_orion_settimer,
	_orion_settimermode,
	default_handler, /* handler */
	RTL_CLOCK_MODE_ONESHOT, /* mode */
};


static unsigned long gettimeoffset(void)
{
	u32 count;
	unsigned long res, tmp;

	/* Last jiffy when do_fast_gettimeoffset() was called. */
	static unsigned long last_jiffies=0;
	unsigned long quotient;

	/*
	 * Cached "1/(clocks per usec)*2^32" value.
	 * It has to be recalculated once each jiffy.
	 */
	static unsigned long cached_quotient=0;

	tmp = jiffies;

	quotient = cached_quotient;
#define USECS_PER_JIFFY ((NSECS_PER_SEC/HZ)/1000)
	
	if (tmp && last_jiffies != tmp) {
		last_jiffies = tmp;
		__asm__(".set\tnoreorder\n\t"
			".set\tnoat\n\t"
			".set\tmips3\n\t"
			"lwu\t%0,%2\n\t"
			"dsll32\t$1,%1,0\n\t"
			"or\t$1,$1,%0\n\t"
			"ddivu\t$0,$1,%3\n\t"
			"mflo\t$1\n\t"
			"dsll32\t%0,%4,0\n\t"
			"nop\n\t"
			"ddivu\t$0,%0,$1\n\t"
			"mflo\t%0\n\t"
			".set\tmips0\n\t"
			".set\tat\n\t"
			".set\treorder"
			:"=&r" (quotient)
			:"r" (timerhi),
			 "m" (timerlo),
			 "r" (tmp),
			 "r" (USECS_PER_JIFFY)
			:"$1");
		cached_quotient = quotient;
	}

	/* Get last timer tick in absolute kernel time */
	count = read_32bit_cp0_register(CP0_COUNT);

	/* .. relative to previous jiffy (32 bits is enough) */
	count -= timerlo;

	__asm__("multu\t%1,%2\n\t"
		"mfhi\t%0"
		:"=r" (res)
		:"r" (count),
		 "r" (quotient));

	/*
 	 * Due to possible jiffies inconsistencies, we need to check 
	 * the result so that we'll get a timer that is monotonic.
	 */
	if (res >= USECS_PER_JIFFY)
		res = USECS_PER_JIFFY-1;

	return res;
}

hrtime_t _orion_gettime (struct rtl_clock *clock)
{
	hrtime_t ret;

	ret = (orion_clock.arch.time*((NSECS_PER_SEC/HZ)/(50000000/HZ))) + (gettimeoffset()*1000);
	return ret;
}

static int _orion_settimer (struct rtl_clock *clock, hrtime_t interval)
{
	__u32 val;

	/*
	 * If we're going to lose precision we know that the # ticks
	 * will be greater than a jiffy.
	 *  -- Cort
	 */
	if ( (ulong)(interval>>32) )
	{
		val = 0xffffffff;
	}
	else
	{
		/* ns to ticks -- Cort */
		val = (long)interval / ((NSECS_PER_SEC/HZ)/(50000000/HZ)) + 1;
		if (orion_clock.mode == RTL_CLOCK_MODE_PERIODIC)
			clock->resolution = interval;
		else
			clock->resolution = 1;
	}
	
	if ( val > (50000000/HZ) )
	{
		/* time was too high, set timer to hit when Linux wants its next tick */
		if ( (50000000/HZ) > linux_decrs )
			val = (50000000/HZ) - linux_decrs;
		else
			val = (50000000/HZ);
	}
	else if ( val < 50 )
	{
		val = 50;
	}
	
	/* so we know how many linux ticks have gone by */
	orion_tick_count = val;
	
	orion_clock.arch.istimerset = 1;

	*((__u32 *) (((unsigned)(0x14000000)|0xA0000000) + 0x864)) =
		cpu_to_le32(0);
	
	*((__u32 *) (((unsigned)(0x14000000)|0xA0000000) + 0x850)) =
		cpu_to_le32(0);

	*((__u32 *) (((unsigned)(0x14000000)|0xA0000000) + 0x850)) =
		cpu_to_le32(val);

	*((__u32 *) (((unsigned)(0x14000000)|0xA0000000) + 0xC1C)) =
		cpu_to_le32(0x100);
     
	*((__u32 *) (((unsigned)(0x14000000)|0xA0000000) + 0x864)) =
		cpu_to_le32(0x03);
     
	*((__u32 *) (((unsigned)(0x14000000)|0xA0000000) + 0xC18)) =
		cpu_to_le32(0);
	
	return 0;
}

static int _orion_settimermode(struct rtl_clock *clock, int mode)
{
	clock->mode = mode;
	return 0;
}

clockid_t rtl_getbestclock (unsigned int cpu)
{
        return &orion_clock;
}

hrtime_t gethrtime(void)
{
	return _orion_gettime(&orion_clock);
}

hrtime_t gethrtimeres(void)
{
	return (NSECS_PER_SEC/HZ)/(50000000/HZ);
}

static unsigned int _orion_timer_intercept( unsigned int irq,
					    struct pt_regs *regs )
{
	void (*handler)( struct pt_regs *regs) = orion_clock.handler;
	unsigned int count;

	/* update the time in the clock */
	orion_clock.arch.time += orion_tick_count;
	
	count = read_32bit_cp0_register(CP0_COUNT);
	timerhi += (count < timerlo);	/* Wrap around */
	timerlo = count;

	if ( (orion_clock.mode == RTL_CLOCK_MODE_ONESHOT) && orion_clock.arch.istimerset )
		orion_clock.arch.istimerset = 0;
	
	/* give the interrupt to Linux */
	linux_decrs += orion_tick_count;
	if ( linux_decrs >= (50000000/HZ) )
	{
		if ( !rtl_global_ispending_irq(2) )
		{
			rtl_global_pend_irq( 2 );
			linux_decrs -= (50000000/HZ);
		}
	}

	if ( handler != default_handler )
		handler(regs);

	orion_clock.arch.istimerset = 1; // what's this for? -- Michael
	return 0;
}

static int _orion_init (clockid_t clock)
{
	/* on next tick, how many have gone by */
	orion_tick_count = (50000000/HZ);
	rtl_request_global_irq( 2, _orion_timer_intercept );
	return 0;
}

static void _orion_uninit (clockid_t clock)
{
	clock->handler = RTL_CLOCK_DEFAULTS.handler;
	rtl_free_global_irq( 2 );
}

int init_module (void)
{
	rtl_init_standard_clocks();
	return 0;
}

void cleanup_module(void)
{
	rtl_cleanup_standard_clocks();
	_orion_uninit(&orion_clock);
}
