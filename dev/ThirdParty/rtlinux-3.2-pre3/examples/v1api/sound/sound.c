#include <linux/module.h>

#include <rtl_fifo.h>
#include <asm/rt_irq.h>
#include <linux/rt_time.h>
#include <linux/mc146818rtc.h>
#include <linux/cons.h>


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


void intr_handler(void) {
	char data;
	char temp;
 	(void) CMOS_READ(RTC_REG_C);        /* clear IRQ */ 
	if (rtf_get(0, &data, 1) > 0) {
		data = filter(data);
/* 		if (data) conpr("1"); else conpr("0"); */
		temp = inb(0x61);            
		temp &= 0xfc;
		if (data) {
			temp |= 3;
		}
		outb(temp,0x61);
	}
}

char save_cmos_A;
char save_cmos_B;


int init_module(void)
{
	char ctemp;
	if (!I8253_channel2_free()) {
		conpr("RTLinux sound: can't use channel 2; bailing out\n");
		return -1;
	}
	rtf_create(0, 4000);

	/* this is just to ensure that the output of the counter is 1 */
	outb_p(0xb0, 0x43);	/* binary, mode 0, LSB/MSB, ch 2 */
	outb_p(0x1, 0x42);
	outb_p(0x0, 0x42);

	request_RTirq(8, intr_handler);

	/* program the RTC to interrupt at 8192 Hz */
	save_cmos_A = CMOS_READ(RTC_REG_A);
	save_cmos_B = CMOS_READ(RTC_REG_B);

	CMOS_WRITE(0x23, RTC_REG_A);  	/* 32kHz Time Base, 8192 Hz interrupt frequency */
	ctemp = CMOS_READ(RTC_REG_B);
	ctemp &= 0x8f;                	/* Clear */
	ctemp |= 0x40;                	/* Periodic interrupt enable */
	CMOS_WRITE(ctemp, RTC_REG_B); 

	(void) CMOS_READ(RTC_REG_C);

	return 0;
} 


void cleanup_module(void)
{
	rtf_destroy(0);
	outb_p(0xb6, 0x43);	/* restore the original mode */
	CMOS_WRITE(save_cmos_A, RTC_REG_A);
	CMOS_WRITE(save_cmos_B, RTC_REG_B);
	free_RTirq(8);
}

