/*
 * Alpha-specific clock support
 *
 * Copyright (C) 2000 FSM Labs (http://www.fsmlabs.com/)
 *  Written by Cort Dougan <cort@fsmlabs.com>
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/timex.h>
#include <linux/mc146818rtc.h>

#include <asm/smp.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/hwrpb.h>

#include <rtl_core.h>
#include <rtl_time.h>
#include <rtl_sync.h>
#include <rtl.h>

MODULE_AUTHOR("Cort Dougan <cort@fsmlabs.com>");
MODULE_DESCRIPTION("RTLinux Alpha Timer Module");

static unsigned long hrtime_last_tick = 0;
static __u32 cycles_last_tick = 0;
struct rtl_clock _i8254_clock;
static unsigned int clock_counter; /* current latch value */

#define wait_value(x) do {; } while ((inb(0x61) & 0x20) != (x))
#define wait_cycle() do { wait_value(0); wait_value(0x20); } while (0)

#define WRITE_COUNTER_ZERO16(x)  { \
	outb_p(x&0xff,0x40); outb_p((x>>8)&0xff,0x40);\
      	clock_counter =x; \
} 

#define WRITE_COUNTER_ZERO8(x)  { \
	outb_p(x&0xff,0x40); \
	clock_counter =x; \
}

#define WRITE_COUNTER_ZERO_ONESHOT(x) WRITE_COUNTER_ZERO16(x)

#define CLATCH (1024 * 32)
#define NLOOPS 50

#define READ_CNT0(var) \
{ var = inb(0x40); var |= (inb(0x40) << 8); }

#define READ_CNT2(var) \
{ var = inb(0x42); var |= (inb(0x42) << 8); }

#define LATCH2 0x8000

#define LATCH_CNT0() \
outb(0xd2, 0x43);

#define LATCH_CNT0_AND_2() \
outb(0xda, 0x43);

#define LATCH_CNT2() \
outb(0xd8, 0x43);

static inline long RTIME_to_8254_ticks(long t)
{
	return t / (NSECS_PER_SEC / CLOCK_TICK_RATE);
}

static int _8254_setperiodic (clockid_t c, hrtime_t interval)
{
	long t;

	t = RTIME_to_8254_ticks (interval) + 1;
	if(t < 2)
		t = 2;
	else if ( t > 0xffff )
		t = 0xffff;
	outb_p(0x34,0x43);              /* binary, mode 2, LSB/MSB, ch 0 */
	WRITE_COUNTER_ZERO16 (t);
	
	_i8254_clock.value = gethrtime();
	_i8254_clock.resolution = interval;
	_i8254_clock.arch.istimerset = 1;
	return 0;
}
	
static int _8254_setoneshot (clockid_t c, hrtime_t interval)
{
	long t;

	t = RTIME_to_8254_ticks (interval);

	if (t < 1) 
		t = 1;
	else if (t > 0xffff)
		t = 0xffff;
	
	WRITE_COUNTER_ZERO_ONESHOT(t);
	_i8254_clock.arch.istimerset = 1;
	return 0;
}

int _8254_settimermode (struct rtl_clock *c, int mode)
{
	if (mode == RTL_CLOCK_MODE_PERIODIC) {
		outb_p(0x30, 0x43);/* 8254, channel 0, mode 0, lsb+msb */
		outb_p(0x34, 0x43); /* binary, mode 2, LSB/MSB, ch 0 */
		_i8254_clock.mode = mode;
		_i8254_clock.settimer = _8254_setperiodic;
		_i8254_clock.arch.count_irqs = 0;
	} else if (mode == RTL_CLOCK_MODE_ONESHOT) {
		outb_p(0x30, 0x43);    /* 8254, channel 0, mode 0, lsb+msb */
		_i8254_clock.mode = mode;
		_i8254_clock.settimer = _8254_setoneshot;
		_i8254_clock.resolution = HRTICKS_PER_SEC / CLOCK_TICK_RATE;
	} else {
		return -EINVAL;
	}
	return 0;
}

static unsigned int _8254_irq(unsigned int irq, struct pt_regs *regs)
{
	/* keep track of the cycle timer */
	__u32 x;
	rdtscl(x);
	hrtime_last_tick += ((hrtime_t)(x - cycles_last_tick) * (hrtime_t)NSECS_PER_SEC) / (ulong)hwrpb->cycle_freq;
	cycles_last_tick = x;

	if (_i8254_clock.mode == RTL_CLOCK_MODE_PERIODIC) {
		if (test_and_set_bit(0, &_i8254_clock.arch.count_irqs)) {
			_i8254_clock.value += _i8254_clock.resolution;
		}
	} else {
		_i8254_clock.arch.istimerset = 0;
	}
	rtl_hard_enable_irq(0);
	_i8254_clock.handler(regs);
	return 0;
}

static int _8254_init (clockid_t clock)
{
	int flags;
	rtl_no_interrupts (flags);
	rdtscl(cycles_last_tick);
	
	if ( rtl_request_global_irq(0, _8254_irq) )
	{
		printk("_8254_init(): failed to get irq for timer\n");
		return -1;
	}
	_8254_settimermode (clock, RTL_CLOCK_MODE_ONESHOT);
	rtl_restore_interrupts (flags);
	return 0;
}

static void _8254_uninit (clockid_t clock)
{
	if (clock -> mode == RTL_CLOCK_MODE_UNINITIALIZED) {
		return;
	}
	clock->handler = RTL_CLOCK_DEFAULTS.handler;
	rtl_free_global_irq(0);
	clock -> mode = RTL_CLOCK_MODE_UNINITIALIZED;
}

hrtime_t _gethrtime(struct rtl_clock *c)
{
	return gethrtime();
}

hrtime_t gethrtime(void)
{
	u32 x = 0;
	rdtscl(x);
	x -= cycles_last_tick;
	return (((ulong)x * (ulong)NSECS_PER_SEC) / (ulong)hwrpb->cycle_freq)
		+ hrtime_last_tick;
}

hrtime_t gethrtimeres(void)
{
	return NSECS_PER_SEC / hwrpb->cycle_freq;
}

int rtl_create_clock_8254(void)
{
	_i8254_clock = RTL_CLOCK_DEFAULTS;
	_i8254_clock.gethrtime = _gethrtime;
	_i8254_clock.init = _8254_init;
	_i8254_clock.uninit = _8254_uninit;
	_i8254_clock.settimermode = _8254_settimermode;
	return 0;
}

clockid_t rtl_getbestclock (unsigned int cpu)
{
	return &_i8254_clock;
}

int init_module (void)
{
	rtl_create_clock_8254();
	rtl_init_standard_clocks();
	return 0;
}

void cleanup_module(void)
{
	rtl_cleanup_standard_clocks();
	_8254_uninit(&_i8254_clock);
}
