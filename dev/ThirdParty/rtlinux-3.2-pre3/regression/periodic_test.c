/*
 * (C) Finite State Machine Labs Inc. 1999 business@fsmlabs.com
 *
 * Released under the terms of GPL 2.
 * Open RTLinux makes use of a patented process described in
 * US Patent 5,995,745. Use of this process is governed
 * by the Open RTLinux Patent License which can be obtained from
 * www.fsmlabs.com/PATENT or by sending email to
 * licensequestions@fsmlabs.com
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
#include "common.h"


MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("FSMLabs Inc.");

clockid_t clock;
void shutdown(void);
static int shutdown_flag = 0;
static unsigned long max_diff = 0;
static unsigned long min_diff = -1;
static unsigned long last_time = 0;
static unsigned long count = 0;
#define FIFO_NUM 0

struct timespec spec;


void handler(struct pt_regs *p)
{
	unsigned long x, diff;
	struct sample samp;
	switch (shutdown_flag) {
	case 0:
		rdtscl(x);
		if (last_time) {
			diff = (last_time < x ? x - last_time : 0);	//ignore overflows
			if (diff > max_diff)
				max_diff = diff;
			if (diff & (diff < min_diff))
				min_diff = diff;
		}
		if (count++ > 50) {
			count = 0;
			samp.min = timespec_from_ns(min_diff);
			samp.max = timespec_from_ns(max_diff);
			rtf_put(FIFO_NUM, &samp, sizeof(samp));
			max_diff = 0;
			min_diff = -1;
		}
		rdtscl(last_time);
		break;
	case 1:
		shutdown_flag = 2;
	default:		/* do nothing */ ;

	}
	return;
}

int fifo_size = 4000;
int init_module(void)
{
	rtl_irqstate_t old_irq_state;
	int fifo_status;
	rtf_destroy(FIFO_NUM);	/* just in case someone else is using! */
	fifo_status = rtf_create(FIFO_NUM, fifo_size);
	if (fifo_status) {
		/*printk("RTLinux measurement test fail. fifo_status=%d\n",fifo_status); */
		return -1;
	}
	/*printk("Starting Timer measurement module: numbers in TSC ticks!\n"); */
	rtl_no_interrupts(old_irq_state);
	clock = rtl_getbestclock(rtl_getcpuid());
	if (!clock || rtl_setclockhandler(clock, handler)) {
		/*printk("Can't get clock\n"); */
		rtf_destroy(FIFO_NUM);
		return -1;
	}

	/*printk("Requested timer and got %x\n",(unsigned int)clock); */
	clock->init(clock);
	clock->settimermode(clock, RTL_CLOCK_MODE_PERIODIC);
	clock->settimer(clock, 1000000);
	rtl_restore_interrupts(old_irq_state);
	return 0;
}

void shutdown()
{
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
	while ((shutdown_flag == 1) && timeout--);
	/* so it's kinda sloppy, wait for timeout or for 
	   interrupt routine to ack shutdown, whatever comes
	   first */
	shutdown();
}
