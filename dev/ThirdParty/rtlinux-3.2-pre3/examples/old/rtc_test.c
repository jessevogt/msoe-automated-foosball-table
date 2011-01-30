
/*  RTC_MEASURE
    uses real time clock to generate an interrupt and
    figure out elapsed time
    Originally by Michael "FZ" Barabanov(c) 1997, Released under the GPL.
    Modified by Victor Yodaiken (c) 1998, Released under the GPL.
    */

#include <linux/module.h>
#include <linux/kernel.h>
#include <rtl.h>
#include <asm/ptrace.h>
#include <rtl_core.h>
#include <asm/io.h>
#include <rtl_sync.h>
#include "common.h"


#include <linux/mc146818rtc.h>
char save_cmos_A;
char save_cmos_B;
void shutdown(void);
static int shutdown_flag = 0;
static  unsigned long max_diff = 0;
static  unsigned long last_time = 0;
static  unsigned long count = 0;


unsigned int  handler(unsigned int irq_number, struct pt_regs *p) {
	/* Do not	rt_task_wait() in a handler */
	unsigned long x,diff;
	switch(shutdown_flag){
		case 0:
			rdtscl(x);
			if(last_time){
				diff = (last_time< x? x- last_time : 0); //ignore overflows
				if(diff > max_diff)max_diff=diff;
			}
			if(count++ > 10000){
				count = 0;
				conpr("MAXDIFF="); conprn(max_diff); conpr("\n");
			}

			(void) CMOS_READ(RTC_REG_C);  /* clear IRQ */ 
			rdtscl(last_time); /* after the conpr */
			break;
		case 1: 
			shutdown_flag = 2;
		default: /* do nothing */

	}
	rtl_hard_enable_irq(8);
	return 0;

}


int init_module(void)
{
	unsigned char ctemp;
	int old_irq_state;
	int debug;
        printk("Starting parallel port module\n");
	rtl_no_interrupts(old_irq_state);
	debug = rtl_request_global_irq(8, handler);
	printk("Requested 8 and got %d\n",debug);
	/* program the RTC to interrupt at 8192 Hz */
	save_cmos_A = CMOS_READ(RTC_REG_A);
	save_cmos_B = CMOS_READ(RTC_REG_B);

	CMOS_WRITE(0x23, RTC_REG_A);  	/* 32kHz Time Base, 8192 Hz interrupt frequency */
	ctemp = CMOS_READ(RTC_REG_B);
	ctemp &= 0x8f;                	/* Clear */
	ctemp |= 0x40;                	/* Periodic interrupt enable */
	CMOS_WRITE(ctemp, RTC_REG_B); 

	(void) CMOS_READ(RTC_REG_C);
	rtl_hard_enable_irq(8);
	rtl_restore_interrupts(old_irq_state);
	return 0;
}

void shutdown(){
	int old_irq_state;
	rtl_no_interrupts(old_irq_state);
	CMOS_WRITE(save_cmos_A, RTC_REG_A);
	CMOS_WRITE(save_cmos_B, RTC_REG_B);
	rtl_free_global_irq(8);
	rtl_restore_interrupts(old_irq_state);
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
