/*
 * Periodic timing measurement for RTLinux.
 *
 * Copyright (C) 1999 FSM Labs (http://www.fsmlabs.com/)
 *  Written by Cort Dougan <cort@fsmlabs.com>
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/ptrace.h>
#include <rtl_core.h>
#include <asm/io.h>
#include <rtl_sync.h>
#include <rtl_fifo.h>
#include <rtl_time.h>
#include <rtl_core.h>

clockid_t clock;
void shutdown(void);
static int shutdown_flag = 0;
hrtime_t last_time;
#define FIFO_NUM 0

struct samp
{
	hrtime_t min, max, total;
	hrtime_t period;
	int cnt;
};
struct samp sm;
int period_cnt = 0;

hrtime_t period;

void handler(struct pt_regs *p)
{
	hrtime_t x, y;
	switch( shutdown_flag )
	{
		case 0:
			if ( last_time )
			{
				y = x = gethrtime();
				if ( last_time > x )
					goto out;
				x = x - last_time;
				if ( (ulong)sm.max < (ulong)x )
					sm.max = x;
				if ( (ulong)sm.min > (ulong)x )
					sm.min = x;
				sm.total += x;
				if( sm.cnt++ > 100 )
				{
					sm.period = period;
					rtf_put( FIFO_NUM, &sm, sizeof(sm) );
					sm.cnt = 0;
					sm.max = 0;
					sm.min = HRTIME_INFINITY;
					sm.total = 0;
				}
			}
out:
			clock->settimer(clock, period);
			last_time = gethrtime();
			break;
		case 1: 
			shutdown_flag = 2;
			break;
		default: /* do nothing */
			break;
	}
	return;
}


int fifo_size=4000;
int init_module(void)
{
	rtl_irqstate_t old_irq_state;
	int fifo_status; 
	rtf_destroy(FIFO_NUM); /* just in case someone else is using!*/
	fifo_status = rtf_create(FIFO_NUM, fifo_size);
	if (fifo_status) {
		rtl_printf("RTLinux measurement test fail. fifo_status=%d\n",fifo_status);
		return -1;
	}
        rtl_printf ("Starting Timer measurement module\n");
	rtl_no_interrupts(old_irq_state);
	clock = rtl_getbestclock(rtl_getcpuid());
	if (!clock || rtl_setclockhandler(clock, handler)) {
		rtl_printf("Can't get clock\n");
		rtf_destroy(FIFO_NUM); 
		goto out;
	}
	
	sm.cnt = 0;
	sm.max = 0;
	sm.min = HRTIME_INFINITY;
	sm.total = 0;
	last_time = 0;
	period = 1000000;
	clock->init(clock);
	clock->settimermode(clock, RTL_CLOCK_MODE_ONESHOT);
	clock->settimer(clock, period);
	rtl_restore_interrupts(old_irq_state);
	return 0;
 out:
	rtl_restore_interrupts(old_irq_state);
	return -1;
}

void shutdown(){
	rtl_irqstate_t old_irq_state;
	rtl_no_interrupts(old_irq_state);
	clock->uninit(clock);
	rtl_restore_interrupts(old_irq_state);
	rtf_destroy(FIFO_NUM);
}


void cleanup_module(void)
{
	int timeout = 100000;
	shutdown_flag = 1;
	while((shutdown_flag==1) && timeout--);
	/* so it's kinda sloppy, wait for timeout or for 
	   interrupt routine to ack shutdown, whatever comes
	   first */
	shutdown();
}
