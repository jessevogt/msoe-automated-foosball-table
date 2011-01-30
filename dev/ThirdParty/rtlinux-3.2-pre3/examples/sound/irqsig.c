/*
 * Interrupt handling in Real-Time. Play sounds with PC speaker.
 * Signal version
 *
 * (C) Michael Barabanov, 1997
 *  (C) FSMLabs  1999. baraban@fsmlabs.com
 *  Released under the GNU GENERAL PUBLIC LICENSE Version 2, June 1991
 *  Any use of this code must include this notice.
 *
 */

#include <linux/mc146818rtc.h>

#include <rtl_fifo.h>
#include <rtl_core.h>
#include <rtl_time.h>
#include <rtl.h>
#include <signal.h>
#include <sys/utsname.h>
#include <time.h>

#define FIFO_NO 3
#define RTC_IRQ 8

static int filter(int x)
{
	static int oldx;
	int ret;

	if (x & 0x80) {
		x = 382 - x;
	}
	ret = x > oldx;
	oldx = x;
	return ret;
	
}


void intr_handler(int sig) {
	char data;
	char temp;
 	(void) CMOS_READ(RTC_REG_C);        /* clear IRQ */ 
	if (rtf_get(FIFO_NO, &data, 1) > 0) {
		data = filter(data);
/* 		if (data) conpr("1"); else conpr("0"); */
		temp = inb(0x61);            
		temp &= 0xfc;
		if (data) {
			temp |= 3;
		}
		outb(temp,0x61);
	}
	rtl_hard_enable_irq (RTC_IRQ);
}

char save_cmos_A;
char save_cmos_B;


int init_module(void)
{
	char ctemp;
	int ret;
	time_t t;

	struct utsname name;
	ret = uname(&name);
/* 	rtl_printf("sysname %s, version %s\n", name.sysname, name.version); */

	t = time(NULL);
/* 	rtl_printf("time() returned %d\n", t); */

	if (!I8253_channel2_free()) {
		conpr("RTLinux sound: can't use channel 2; bailing out\n");
		return -1;
	}
	rtf_create(FIFO_NO, 4000);

	/* this is just to ensure that the output of the counter is 1 */
	outb_p(0xb0, 0x43);	/* binary, mode 0, LSB/MSB, ch 2 */
	outb_p(0x1, 0x42);
	outb_p(0x0, 0x42);

	{
		struct sigaction act;
		act.sa_handler = intr_handler;
		act.sa_flags = SA_FOCUS;
		act.sa_focus = 1 << rtl_getcpuid();
		sigaction (RTL_SIGIRQMIN + RTC_IRQ, &act, NULL);
	}

	/* program the RTC to interrupt at 8192 Hz */
	save_cmos_A = CMOS_READ(RTC_REG_A);
	save_cmos_B = CMOS_READ(RTC_REG_B);

	CMOS_WRITE(0x23, RTC_REG_A);  	/* 32kHz Time Base, 8192 Hz interrupt frequency */
	ctemp = CMOS_READ(RTC_REG_B);
	ctemp &= 0x8f;                	/* Clear */
	ctemp |= 0x40;                	/* Periodic interrupt enable */
	CMOS_WRITE(ctemp, RTC_REG_B); 

	rtl_hard_enable_irq (RTC_IRQ);
	(void) CMOS_READ(RTC_REG_C);

	return 0;
} 


void cleanup_module(void)
{
	rtf_destroy(FIFO_NO);
	outb_p(0xb6, 0x43);	/* restore the original mode */
	CMOS_WRITE(save_cmos_A, RTC_REG_A);
	CMOS_WRITE(save_cmos_B, RTC_REG_B);
	{
		struct sigaction act;
		act.sa_handler = SIG_IGN;
		act.sa_flags = 0;
		sigaction (RTL_SIGIRQMIN + RTC_IRQ, &act, NULL);
	}

}

