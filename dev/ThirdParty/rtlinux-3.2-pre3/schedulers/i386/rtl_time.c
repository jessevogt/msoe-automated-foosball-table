/*
 * rtl_time.c
 *
 * architecture-dependent clock support
 *
 * Written by Michael Barabanov
 * Copyright  Finite State Machine Labs Inc. 1998-1999
 * Released under the terms of the GPL Version 2
 *
 */

#include <rtl_conf.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/config.h>
#include <asm/smp.h>
#include <asm/io.h>
#include <linux/errno.h>
#include <asm/system.h>
#include <linux/irq.h>
#include <asm/hw_irq.h>
#include <linux/sched.h>
#include <linux/timex.h>
#include <linux/mc146818rtc.h>

#include <rtl.h>
#include <rtl_core.h>
#include <rtl_debug.h>
#include <rtl_sync.h>
#include <rtl_time.h>

int notsc=0;
MODULE_PARM(notsc,"i");
int _8254_latency = 0;

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("RTLinux/GPL hrtime functions");

#ifndef cpu_has_tsc
#define cpu_has_tsc \
	(boot_cpu_data.x86_capability & X86_FEATURE_TSC)
#endif

hrtime_t _gethrtime(struct rtl_clock *c)
{
	return gethrtime();
}


struct rtl_clock _i8254_clock;
#ifdef CONFIG_X86_LOCAL_APIC
struct rtl_clock _apic_clock[NR_CPUS];
#endif


extern void (*kd_mksound)(unsigned int hz, unsigned int ticks);

static hrtime_t (*rtl_do_get_time)(void);
static hrtime_t hrtime_resolution;

static long LATCH_NS;
static long MAX_LATCH_ONESHOT;

/* getting global time from 8254 */

#define READ_CNT0(var) \
do { var = inb(0x40); var |= (inb(0x40) << 8); } while (0)

#define READ_CNT2(var) \
do { var = inb(0x42); var |= (inb(0x42) << 8); } while (0)


#define LATCH_CNT0() \
outb(0xd2, 0x43);

#define LATCH_CNT0_AND_2() \
outb(0xda, 0x43);

#define LATCH_CNT2() \
outb(0xd8, 0x43);

#define WRITE_COUNTER_ZERO16(x) do { \
	outb(x&0xff,0x40); outb((x>>8)&0xff,0x40);\
      	clock_counter =x; \
} while (0)

#define WRITE_COUNTER_ZERO8(x) do { \
	outb(x&0xff,0x40); \
	clock_counter =x; \
} while (0)

#define WRITE_COUNTER_ZERO_ONESHOT(x) WRITE_COUNTER_ZERO16(x)

static volatile int last_c2;
unsigned long scaler_8254_to_hrtime;  /* =8380965  ns*100 */
unsigned long scaler_hrtime_to_8254;
#define LATCH2 0x8000

/*static */spinlock_t lock8254;
/*static */spinlock_t lock_linuxtime;

hrtime_t gethrtime(void)
{
	return rtl_do_get_time();
}

hrtime_t gethrtimeres(void)
{
	return hrtime_resolution;
}

hrtime_t base_time;
hrtime_t last_8254_time;
long offset_time;
hrtime_t global_8254_gettime (void)
{
	register unsigned int c2;
	int flags;
	long t;

	rtl_spin_lock_irqsave (&lock8254, flags);
	LATCH_CNT2();
	READ_CNT2(c2);
	offset_time += ((c2 < last_c2) ? (last_c2 - c2) / 2 : (last_c2 - c2 + LATCH2) / 2);
	last_c2 = c2;
	if (offset_time >= CLOCK_TICK_RATE) {
		offset_time -= CLOCK_TICK_RATE;
		base_time += HRTICKS_PER_SEC;
	}

#if HRTICKS_PER_SEC != CLOCK_TICK_RATE
	__asm__("shl $10, %%eax\n\t"
		"mul %%ebx\n\t"
	:"=d" (t) : "b" (scaler_8254_to_hrtime), "a" (offset_time));
#else
	t = offset_time;
#endif
	last_8254_time = base_time + t;
	rtl_spin_unlock_irqrestore (&lock8254, flags);
	return last_8254_time;
}


/* getting global time from Pentium TSC */
static unsigned long scaler_pentium_to_hrtime = 0;
int can_change_latch2;

int I8253_channel2_free(void)
{
	return can_change_latch2;
}


hrtime_t pent_gettime(void)
{

	hrtime_t t;

	/* time = counter * scaler_pentium_to_hrtime / 2^32 * 2^5; */
	/* Why 2^5? Because the slowest Pentiums run at 60 MHz */

	__asm__("rdtsc\n\t"
		"mov %%edx, %%ecx\n\t"
		"mul %%ebx\n\t"  	/* multiply the low 32 bits of the counter by the scaler_pentium */
		"mov %%ecx, %%eax\n\t"
		"mov %%edx, %%ecx\n\t"	/* save the high 32 bits of the product */
		"mul %%ebx\n\t" 	/* now the high 32 bits of the counter */
		"add %%ecx, %%eax\n\t"
		"adc $0, %%edx\n\t"
#if HRTICKS_PER_SEC == NSECS_PER_SEC
		"shld $5, %%eax, %%edx\n\t"
		"shl $5, %%eax\n\t"
#endif
		:"=A" (t) : "b" (scaler_pentium_to_hrtime) : "cx");
	return t;
}

extern unsigned long (*do_gettimeoffset)(void);
static unsigned long (*save_do_gettimeoffset)(void);
/* we can't allow Linux to read the clocks; it must use this function */
#include <linux/timex.h>
#define TICK_SIZE tick
#define printll(x) rtl_printf("%x%08x ", (unsigned) ((x) >> 32), (unsigned) (x))

static int rtl_save_jiffies;

// start of the current linux tick wrt gethrtime()
static inline hrtime_t rtl_get_base(void)
{
	long flags;
	hrtime_t base;

	rtl_spin_lock_irqsave (&lock_linuxtime, flags);
	base = _i8254_clock.arch.linux_time - LATCH_NS;
	if (jiffies == rtl_save_jiffies) {
		base -= LATCH_NS;
	}
	rtl_spin_unlock_irqrestore (&lock_linuxtime, flags);
	return base;
}

static unsigned long do_rt_gettimeoffset(void)
{
	int count;
	long diff;

	diff = (long) (gethrtime() - rtl_get_base());
	count = muldiv(diff, TICK_SIZE, LATCH_NS);

	return count;
}

#ifdef CONFIG_RTL_CLOCK_GPOS

extern rwlock_t xtime_lock;

#if LINUX_2_4_0_FINAL_OR_LATER
extern unsigned long wall_jiffies;
#define lost_ticks (jiffies - wall_jiffies)
#else
extern volatile unsigned long lost_ticks;
#endif

void rtl_clock_gpos_update(void);
static hrtime_t gpos_get_delta(void)
{
	unsigned long flags;
	hrtime_t delta;
	read_lock_irqsave(&xtime_lock, flags);
	delta = (hrtime_t) xtime.tv_sec * NSECS_PER_SEC + xtime.tv_usec * 1000;
	delta += lost_ticks * (NSECS_PER_SEC / HZ);
	delta -= rtl_get_base();
	read_unlock_irqrestore(&xtime_lock, flags);
	return delta;
}

#else
#define rtl_clock_gpos_update() do { ; } while (0)
#endif


#ifdef CONFIG_VT
static void rtl_kd_nosound(unsigned long ignored)
{
	int flags;
	rtl_no_interrupts(flags);
	outb(inb(0x61) & 0xfd, 0x61);
	rtl_restore_interrupts(flags);
}

static void rtl_kd_mksound(unsigned int hz, unsigned int ticks)
{
	static struct timer_list sound_timer = { function: rtl_kd_nosound };

	unsigned int count = 0;

	if (hz > 20 && hz < 32767)
		count = 1193180 / hz;
	
	cli();
	del_timer(&sound_timer);
	if (count) {
		int flags;
		rtl_spin_lock_irqsave (&lock8254, flags);
		outb(inb(0x61)|3, 0x61);
		if (can_change_latch2) {
			outb(0xB6, 0x43);
			outb(count & 0xff, 0x42);
			outb((count >> 8) & 0xff, 0x42);
		}
		rtl_spin_unlock_irqrestore (&lock8254, flags);

		if (ticks) {
			sound_timer.expires = jiffies+ticks;
			add_timer(&sound_timer);
		}
	} else
		rtl_kd_nosound(0);
	sti();
	return;
}


static void (*save_kd_mksound)(unsigned int hz, unsigned int ticks);
#endif

static void uninit_hrtime (void){
#ifdef CONFIG_VT
	rtl_kd_mksound(0, 0);
	kd_mksound = save_kd_mksound;
#endif
}

#define wait_value(x) do {; } while ((inb(0x61) & 0x20) != (x))
#define wait_cycle() do { wait_value(0); wait_value(0x20); } while (0)

#define CLATCH (1024 * 32)
#define NLOOPS 50

#ifdef CONFIG_X86_LOCAL_APIC
static unsigned long scaler_hrtime_to_apic;
/* this is defined in arch/i386/kernel/smp.c */
#define APIC_DIVISOR 16
static long apic_ticks_per_sec;
#endif

/* scaler_pentium ==  2^32 / (2^5 * (cpu clocks per ns)) */
static void do_calibration(int do_tsc)
{
#ifdef CONFIG_X86_LOCAL_APIC
	long temp = 0;
	long a1 = 0;
	long a2 = 0;
	long save_apic = 0;
	long result_apic;
#endif
	long long t1 = 0;
	long long t2 = 0;
	long pps;
	int j;
	long result = 0;

	rtl_irqstate_t flags;
	rtl_no_interrupts(flags);

#ifdef CONFIG_X86_LOCAL_APIC
	if (smp_found_config) {
		save_apic = apic_read(APIC_TMICT);
		apic_write(APIC_TMICT, 1000000000/APIC_DIVISOR);
	}
#endif

	outb((inb(0x61) & ~0x02) | 0x01, 0x61);

	outb(0xb6, 0x43);     /* binary, mode 3, LSB/MSB, ch 2 */
	outb(CLATCH & 0xff, 0x42);	/* LSB of count */
	outb(CLATCH >> 8, 0x42);	/* MSB of count */

	wait_cycle();
	if (do_tsc)
		rdtscll(t1);
#ifdef CONFIG_X86_LOCAL_APIC
	if (smp_found_config) {
		a1 = apic_read(APIC_TMCCT);
	}
#endif

	for (j = 0; j < NLOOPS; j++) {
		wait_cycle();
/* 		rtl_allow_interrupts(); */
/* 		rtl_stop_interrupts(); */
	}
	if (do_tsc)
		rdtscll(t2);
#ifdef CONFIG_X86_LOCAL_APIC
	if (smp_found_config) {
		a2 = apic_read(APIC_TMCCT);
	}
	result_apic = a1 - a2;
#endif
	if (do_tsc)
		result = t2 - t1;

#ifdef CONFIG_X86_LOCAL_APIC
	if (smp_found_config) {
		temp = apic_read(APIC_TMICT);
		apic_write(APIC_TMICT, save_apic);
	}
#endif

	rtl_restore_interrupts(flags);

	if (do_tsc) {
		pps = muldiv (result, CLOCK_TICK_RATE, CLATCH * NLOOPS);
#if HRTICKS_PER_SEC == NSECS_PER_SEC
		scaler_pentium_to_hrtime = muldiv (1 << 27, HRTICKS_PER_SEC, pps);
#else
		scaler_pentium_to_hrtime = muldiv (1 << 31, HRTICKS_PER_SEC * 2, pps);
#endif
	} else  {
		scaler_pentium_to_hrtime = 0;
	}
#ifdef CONFIG_X86_LOCAL_APIC
	if (smp_found_config) {
		temp = muldiv (result_apic, CLOCK_TICK_RATE, CLATCH * NLOOPS);
#if HRTICKS_PER_SEC == NSECS_PER_SEC
		scaler_hrtime_to_apic = muldiv (temp, 1 << 31, HRTICKS_PER_SEC / 2);
#else
		scaler_hrtime_to_apic = muldiv (temp, 1 << (31 - 10), HRTICKS_PER_SEC / 2);
#endif
/* 		printk("sca=%ld, temp=%ld resapic=%ld\n", scaler_hrtime_to_apic, temp, result_apic); */
 		apic_ticks_per_sec = temp;
 /* 		printk("pps apic=%ld %ld\n", temp * APIC_DIVISOR, scaler_hrtime_to_apic); */

	}
#endif
/* 	printk("pps=%ld\n", pps); */
}

static void init_hrtime (void)
{
	int flags;

#ifdef CONFIG_VT
	kd_mksound(0, 0); /* clear the possibly pending sound timer */
	save_kd_mksound = kd_mksound;
	kd_mksound = rtl_kd_mksound;
#endif

#if HRTICKS_PER_SEC != CLOCK_TICK_RATE
	scaler_8254_to_hrtime = muldiv (HRTICKS_PER_SEC, 1 << 22, CLOCK_TICK_RATE);
	scaler_hrtime_to_8254 = muldiv (CLOCK_TICK_RATE, 1 << 31, HRTICKS_PER_SEC / 2);
	LATCH_NS = muldiv (LATCH, HRTICKS_PER_SEC, CLOCK_TICK_RATE);
#else
	LATCH_NS = LATCH;
#endif
	MAX_LATCH_ONESHOT = LATCH_NS * 3 / 4;

	rtl_no_interrupts(flags);
	if (cpu_has_tsc && !notsc) {
		can_change_latch2 = 1;

		do_calibration(1);

		rtl_do_get_time = pent_gettime;
		hrtime_resolution = 32;
		
/* printk("scaler_pentium_to_hrtime = %d\n", (int) scaler_pentium_to_hrtime); */
	} else {
		do_calibration(0);
		can_change_latch2 = 0;
		/* program channel 2 of the 8254 chip for periodic counting */
		outb(0xb6, 0x43);     /* binary, mode 3, LSB/MSB, ch 2 */
		outb(LATCH2 & 0xff, 0x42);
		outb((LATCH2 >> 8) & 0xff, 0x42);
		outb((inb(0x61) & 0xfd) | 1, 0x61); /* shut up the speaker and enable counting */

		LATCH_CNT2();
		READ_CNT2(last_c2);
		offset_time = 0;
		base_time = 0;
		rtl_do_get_time = global_8254_gettime;
		hrtime_resolution = HRTICKS_PER_SEC / CLOCK_TICK_RATE;
	}


	/*
	current_time.tv_sec = xtime.tv_sec;
	current_time.tv_nsec = 0;

	rtl_time_offset = 0;
	rtl_time_offset = rtl_timespec_to_rtime(&current_time) - rtl_get_time();

	*/
	rtl_restore_interrupts(flags);
}

void rtl_clock_clear(clockid_t h)
{
	h->uninit(h);
}




static hrtime_t periodic_gethrtime (struct rtl_clock *c) { return c->value; }
static hrtime_t oneshot_gethrtime (struct rtl_clock *c) { return gethrtime(); }

/* the 8254 clock */

static unsigned int clock_counter; /* current latch value */


static inline long RTIME_to_8254_ticks(long t)
{
#if HRTICKS_PER_SEC != CLOCK_TICK_RATE
	int dummy;
	__asm__("mull %2"
		:"=a" (dummy), "=d" (t)
		:"g" (scaler_hrtime_to_8254), "0" (t)
		);
#endif
		
	return (t);
}

void _8254_checklinuxirq (void)
{
	rtl_irqstate_t flags;
	hrtime_t t;
	rtl_no_interrupts(flags);

	rtl_spin_lock(&lock_linuxtime);
	t = gethrtime();
	
	if (t > _i8254_clock.arch.linux_time && !rtl_global_ispending_irq(0)) {
		_i8254_clock.arch.linux_time += LATCH_NS;
		rtl_global_pend_irq (0);
		do {
			rtl_save_jiffies = jiffies;
		} while (rtl_save_jiffies != jiffies);

		rtl_clock_gpos_update();
	}
	rtl_spin_unlock(&lock_linuxtime);
#if 0
	else if (t < _i8254_clock.arch.linux_time + LATCH_NS) {
		/* laptops: TSC is reset on suspend/resume */
		_i8254_clock.arch.linux_time = t;
	}
#endif


	if (!_i8254_clock.arch.istimerset) { // TODO move to the default handler
		_i8254_clock.settimer (&_i8254_clock, MAX_LATCH_ONESHOT);
	}

	rtl_restore_interrupts(flags);
}

static unsigned int _8254_irq(unsigned int irq, struct pt_regs *regs)
{
	int flags;
	rtl_spin_lock_irqsave (&lock8254, flags);
	if (_i8254_clock.mode == RTL_CLOCK_MODE_PERIODIC) {
		if (test_and_set_bit(0, &_i8254_clock.arch.count_irqs)) {
			_i8254_clock.value += _i8254_clock.resolution;
		}
	} else {
		_i8254_clock.arch.istimerset = 0;
	}
	rtl_hard_enable_irq(0);
	rtl_spin_unlock_irqrestore (&lock8254, flags);
	_i8254_clock.handler(regs);
	if (rtl_rt_system_is_idle()) {
		rtl_allow_interrupts();
	}
	_8254_checklinuxirq();
	rtl_stop_interrupts();
	return 0;
}


static int _8254_setperiodic (clockid_t c, hrtime_t interval)
{
	int flags;
	long t;

	rtl_spin_lock_irqsave (&lock8254, flags);
	t = RTIME_to_8254_ticks (interval) + 1;
	if (t < 10) {
		t = LATCH;
		rtl_printf("RTLinux 8254 periodic settimer set too low!\n");
	}
	if (t > LATCH) {
		t = LATCH;
		rtl_printf("RTLinux 8254 periodic settimer set too high!\n");
	}
	WRITE_COUNTER_ZERO16 (t);

	_i8254_clock.value = gethrtime();
	_i8254_clock.resolution = interval;
	_i8254_clock.arch.istimerset = 1;
	rtl_spin_unlock_irqrestore(&lock8254, flags);
	return 0;
}
	
static int _8254_setoneshot (clockid_t c, hrtime_t interval)
{
	rtl_irqstate_t flags;
	long t;
	rtl_spin_lock_irqsave (&lock8254, flags);
	if (interval > MAX_LATCH_ONESHOT) {
		interval = MAX_LATCH_ONESHOT;
	}

	t = RTIME_to_8254_ticks (interval); /* - _8254_latency); */
	if (t < 1) {
		t = 1;
	}
	WRITE_COUNTER_ZERO_ONESHOT(t);
	_i8254_clock.arch.istimerset = 1;

	rtl_spin_unlock_irqrestore(&lock8254, flags);
	return 0;
}



int _8254_settimermode (struct rtl_clock *c, int mode)
{
	if (mode == _i8254_clock.mode) {
		return 0;
	}
	if (mode == RTL_CLOCK_MODE_PERIODIC) {
		outb(0x30, 0x43);/* 8254, channel 0, mode 0, lsb+msb */
		outb(0x34, 0x43); /* binary, mode 2, LSB/MSB, ch 0 */
		_i8254_clock.mode = mode;
		_i8254_clock.gethrtime = periodic_gethrtime;
		_i8254_clock.settimer = _8254_setperiodic;
		_i8254_clock.arch.count_irqs = 0;
	} else if (mode == RTL_CLOCK_MODE_ONESHOT) {
		outb(0x30, 0x43);    /* 8254, channel 0, mode 0, lsb+msb */
		_i8254_clock.mode = mode;
		_i8254_clock.gethrtime = oneshot_gethrtime;
		_i8254_clock.settimer = _8254_setoneshot;
		_i8254_clock.resolution = HRTICKS_PER_SEC / CLOCK_TICK_RATE;
/*		{
			int i;
			hrtime_t begin = gethrtime();
			hrtime_t end;
			for (i=0; i < 100; i++) {
				_i8254_clock.settimer(&_i8254_clock, MAX_LATCH_ONESHOT);
			}
			end = gethrtime();
			_8254_latency = (int)(end - begin) / 100;
			rtl_printf("8254 oneshot settimer takes %d\n", _8254_latency);
		} */
	} else {
		return -EINVAL;
	}
	return 0;
}


extern int use_tsc;
int save_use_tsc;
static int _8254_init (clockid_t clock)
{
	int flags;
	rtl_no_interrupts (flags);

/* #ifndef CONFIG_X86_TSC */
	save_do_gettimeoffset = do_gettimeoffset;
	do_gettimeoffset = do_rt_gettimeoffset;
	save_use_tsc = use_tsc;
	use_tsc = 0;
	rtl_save_jiffies = jiffies;
/* #endif */
	_i8254_clock.arch.linux_time = gethrtime() + LATCH_NS;
	rtl_request_global_irq(0, _8254_irq);
	_8254_settimermode (clock, RTL_CLOCK_MODE_ONESHOT);
	_i8254_clock.settimer (clock, HRTIME_INFINITY);
	rtl_restore_interrupts (flags);
	return 0;
}

static void _8254_uninit (clockid_t clock)
{
	int flags;
	if (clock -> mode == RTL_CLOCK_MODE_UNINITIALIZED) {
		return;
	}
	clock->handler = RTL_CLOCK_DEFAULTS.handler;
	rtl_spin_lock_irqsave (&lock8254, flags);
/* #ifndef CONFIG_X88_TSC */
	do_gettimeoffset = save_do_gettimeoffset;
	use_tsc = save_use_tsc;
/* #endif */
	outb(0x34,0x43);		/* binary, mode 2, LSB/MSB, ch 0 */
	WRITE_COUNTER_ZERO16(LATCH);
	rtl_free_global_irq(0);
	clock -> mode = RTL_CLOCK_MODE_UNINITIALIZED;
	rtl_spin_unlock_irqrestore (&lock8254, flags);
}


/* sort of a constructor */
int rtl_create_clock_8254(void)
{
	_i8254_clock = RTL_CLOCK_DEFAULTS;
	_i8254_clock.init = _8254_init;
	_i8254_clock.uninit = _8254_uninit;
	_i8254_clock.settimermode = _8254_settimermode;
	return 0;
}


#ifdef CONFIG_X86_LOCAL_APIC
/* APIC clocks */


static long RTIME_to_apic_ticks(long t)
{
	int dummy;
#if HRTICKS_PER_SEC != NSECS_PER_SEC
	t <<= 10;
#endif
	__asm__("mull %2"
		:"=a" (dummy), "=d" (t)
		:"g" (scaler_hrtime_to_apic), "0" (t)
		);
		
	return (t);
}


static inline int rtl_apic_write_initial_count (long count)
{
	unsigned int tmp_value;
	tmp_value = apic_read(APIC_TMICT);
	apic_write (APIC_TMICT, count);
	return 0;
}

static int apic_setoneshot (clockid_t apic, hrtime_t interval)
{
	long t;
	if (apic != &_apic_clock[rtl_getcpuid()]) {
		rtl_printf("apic_setoneshot crosses CPUs!\n");
		return -1;
	}
	if (interval > MAX_LATCH_ONESHOT) {
		interval = MAX_LATCH_ONESHOT;
	}

	t = RTIME_to_apic_ticks (interval);
	if (t < 1) {
		t = 1;
	}
	rtl_apic_write_initial_count (t);
	apic->arch.istimerset = 1;
	return 0;
}

static int apic_setperiodic (clockid_t apic, hrtime_t interval)
{
	long t;
	t = RTIME_to_apic_ticks (interval);
	rtl_apic_write_initial_count (t);
	apic->value = gethrtime();
	apic->resolution = interval;
	apic->arch.istimerset = 1;
	return 0;
}

int apic_settimermode (struct rtl_clock *apic, int mode)
{
	unsigned long lvtt1_value;
	unsigned int tmp_value;

	if (apic != &_apic_clock[rtl_getcpuid()]) {
		rtl_printf("apic_settimermode crosses CPUs!\n");
		return -EINVAL;
	}

	if (mode == apic->mode) {
		return 0;
	}

	if (mode == RTL_CLOCK_MODE_PERIODIC) {
		apic -> mode = mode;
		apic -> gethrtime = periodic_gethrtime;
		apic -> settimer = apic_setperiodic;
		tmp_value = apic_read(APIC_LVTT);
		lvtt1_value = APIC_LVT_TIMER_PERIODIC | LOCAL_TIMER_VECTOR;
		apic_write(APIC_LVTT , lvtt1_value);
	} else if (mode == RTL_CLOCK_MODE_ONESHOT) {
		apic -> mode = mode;
		apic -> gethrtime = oneshot_gethrtime;
		apic -> settimer = apic_setoneshot;
		apic -> resolution = hrtime_resolution;

		tmp_value = apic_read(APIC_LVTT);
		lvtt1_value = LOCAL_TIMER_VECTOR;
		apic_write(APIC_LVTT , lvtt1_value);

	} else {
		return -EINVAL;
	}
	return 0;
}


static void apic_checklinuxirq (void)
{
	unsigned int cpu_id = rtl_getcpuid();
	clockid_t apic = &_apic_clock[cpu_id];
	hrtime_t t;
#ifdef __RTL_LOCALIRQS__
	if ((1 << cpu_id) & rtl_reserved_cpumask) return;
#endif
	t = apic->gethrtime(apic);
	if  (t > apic->arch.linux_time) {
		rtl_local_pend_vec (LOCAL_TIMER_VECTOR,cpu_id);
		apic->arch.linux_time += LATCH_NS;
		if (t > apic->arch.linux_time) {
			int njiffies = 0;
			do {
				if (++njiffies >= 2 * HZ) {
					apic->arch.linux_time = t;
/* 					rtl_printf("RTL: lost %d linux apic ticks on CPU %d\n", njiffies, cpu_id); */
					break;
				}
				apic->arch.linux_time += LATCH_NS;
			} while (t > apic->arch.linux_time);
		}
	}
}

static unsigned int apic_timer_irq(struct pt_regs *r)
{
	unsigned int cpu_id = rtl_getcpuid();
	clockid_t apic = &_apic_clock[cpu_id];
	if (apic->mode == RTL_CLOCK_MODE_PERIODIC) {
		apic->value += apic->resolution;
	} else {
		apic->arch.istimerset = 0;
	}
	apic -> handler (r);
	apic_checklinuxirq();
	if (!apic -> arch.istimerset) { // TODO move into the default handler
		apic->settimer (apic, MAX_LATCH_ONESHOT);
	}
	return 0;
}


int volatile apic_init_flag;

static unsigned int apic_init_irq(struct pt_regs *r)
{
	unsigned int cpu_id = rtl_getcpuid();
	CLOCK_APIC->settimermode (CLOCK_APIC, RTL_CLOCK_MODE_ONESHOT);
	CLOCK_APIC->settimer (CLOCK_APIC, HRTIME_INFINITY);
	rtl_free_local_irq (LOCAL_TIMER_VECTOR, cpu_id);
	rtl_request_local_irq (LOCAL_TIMER_VECTOR, apic_timer_irq, cpu_id);
	clear_bit (0, &apic_init_flag);
	return 0;
}

/* apics are initialized to the oneshot mode by default */
static int apic_clock_init (clockid_t clk)
{
	int flags;
	unsigned int cpu_id = rtl_getcpuid();

	rtl_no_interrupts (flags);
	clk->arch.linux_time = gethrtime() + LATCH_NS;

	if (clk->arch.apic_cpu == cpu_id) {
		clk->settimermode (clk, RTL_CLOCK_MODE_ONESHOT);
		clk->settimer (clk, HRTIME_INFINITY);
		rtl_request_local_irq (LOCAL_TIMER_VECTOR, apic_timer_irq, cpu_id);
	} else {
		set_bit (0, &apic_init_flag);
		rtl_request_local_irq (LOCAL_TIMER_VECTOR, apic_init_irq, clk->arch.apic_cpu);
		while (test_bit(0, &apic_init_flag));
	}
	rtl_restore_interrupts (flags);
	return 0;
}


static void apic_uninit_handler( struct pt_regs *regs)
{
	unsigned int cpu_id = rtl_getcpuid();
	CLOCK_APIC->settimermode (CLOCK_APIC, RTL_CLOCK_MODE_PERIODIC);
	CLOCK_APIC->settimer (CLOCK_APIC, LATCH_NS);
	CLOCK_APIC -> mode = RTL_CLOCK_MODE_UNINITIALIZED;
	rtl_unsetclockhandler (CLOCK_APIC);
	rtl_free_local_irq (LOCAL_TIMER_VECTOR, cpu_id);
	clear_bit (0, &apic_init_flag);
}

static void apic_clock_uninit (clockid_t clock)
{
	int flags;
	unsigned int cpu_id = rtl_getcpuid();
	rtl_no_interrupts (flags);

	if (clock -> mode == RTL_CLOCK_MODE_UNINITIALIZED) {
		rtl_restore_interrupts (flags);
		return;
	}

	if (clock->arch.apic_cpu == cpu_id) {
		apic_uninit_handler(NULL);
	} else {
		set_bit (0, &apic_init_flag);
		rtl_unsetclockhandler (clock);
		rtl_setclockhandler (clock, apic_uninit_handler);
		while (test_bit(0, &apic_init_flag));
	}
	rtl_restore_interrupts (flags);
}

int rtl_create_clock_apic(int cpu)
{
	_apic_clock[cpu] = RTL_CLOCK_DEFAULTS;
	_apic_clock[cpu].init = apic_clock_init;
	_apic_clock[cpu].uninit = apic_clock_uninit;
	_apic_clock[cpu].settimermode = apic_settimermode;
	_apic_clock[cpu].arch.apic_cpu = cpu;
	return 0;
}

#endif



/* returns a pointer to the clock structure of the best controlling hw clock 
 * for this CPU */
clockid_t rtl_getbestclock (unsigned int cpu)
{
#ifdef CONFIG_X86_LOCAL_APIC
	if (smp_found_config) {
		return &_apic_clock[cpu];
	} else {
		return &_i8254_clock;
	}
#else
	return &_i8254_clock;
#endif
}

#ifdef CONFIG_RTL_CLOCK_GPOS

static struct rtl_clock clock_gpos;
clockid_t CLOCK_GPOS = &clock_gpos;

static hrtime_t gpos_gethrtime(struct rtl_clock *c)
{
	hrtime_t t;
	pthread_spin_lock(&c->lock);
	t = gethrtime() + c->delta;
	pthread_spin_unlock(&c->lock);
	return t;
}

int rtl_clockadjust (clockid_t clock_id, hrtime_t delta)
{
	hrtime_t interdelta = delta - clock_id->delta;
	pthread_spin_lock(&clock_id->lock);
	clock_id->delta += (interdelta >> 3);
	pthread_spin_unlock(&clock_id->lock);

	return 0;
}

static int rtl_clock_gpos_irq = -1;

static void clock_irq_handler (int irq, void *dev_id, struct pt_regs *p)
{
	hrtime_t delta = gpos_get_delta();

	rtl_clockadjust (&clock_gpos, delta);

/*	do_every(300)
       	{
		rtl_printf("%d\n ", (int) clock_id->delta);
	} */
}

static void rtl_clock_gpos_init(void)
{
	rtl_clock_gpos_irq = rtl_get_soft_irq (clock_irq_handler, "RTLinux CLOCK_GPOS");
	clock_gpos = RTL_CLOCK_DEFAULTS;
	clock_gpos.gethrtime = &gpos_gethrtime;
	clock_gpos.resolution = gethrtimeres();
	clock_gpos.delta = gpos_get_delta();
}

static void rtl_clock_gpos_cleanup(void)
{
	rtl_free_soft_irq (rtl_clock_gpos_irq);
}


void rtl_clock_gpos_update(void)
{
	rtl_global_pend_irq (rtl_clock_gpos_irq);
}
#endif

int init_module (void)
{
	rtl_spin_lock_init (&lock8254);
	rtl_spin_lock_init (&lock_linuxtime);

	init_hrtime();
	rtl_create_clock_8254();
#ifdef CONFIG_X86_LOCAL_APIC
	{
	int i;	
		for (i = 0; i < rtl_num_cpus(); i++) {
			int cpu = cpu_logical_map (i);
			rtl_create_clock_apic(cpu);
		}
	}
#ifdef CONFIG_RTL_CLOCK_GPOS
	if (smp_found_config) {
		rtl_irqstate_t flags;
		rtl_no_interrupts(flags);
		_8254_init(&_i8254_clock);
		_i8254_clock.settimermode (&_i8254_clock, RTL_CLOCK_MODE_PERIODIC);
		_i8254_clock.settimer (&_i8254_clock, LATCH_NS);
		rtl_restore_interrupts(flags);
	}
#endif
#endif
	rtl_init_standard_clocks();
#ifdef CONFIG_RTL_CLOCK_GPOS
	rtl_clock_gpos_init();
#endif
	return 0;
}

void cleanup_module(void)
{
#ifdef CONFIG_RTL_CLOCK_GPOS
	rtl_clock_gpos_cleanup();
#endif
	rtl_cleanup_standard_clocks();
#ifdef CONFIG_X86_LOCAL_APIC
	if (smp_found_config) {
		int i;
		for (i = 0; i < rtl_num_cpus(); i++) {
			int cpu = cpu_logical_map(i);
			apic_clock_uninit (&_apic_clock[cpu]);
		}
	}
#endif
	_8254_uninit(&_i8254_clock);
	uninit_hrtime();
}


