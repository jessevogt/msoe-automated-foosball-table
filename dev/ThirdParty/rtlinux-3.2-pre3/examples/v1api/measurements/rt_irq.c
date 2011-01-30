#include <linux/module.h>
#include <asm/rt_irq.h>
#include <asm/io.h>
#include <linux/cons.h>
#include "common.h"

static int output = 0xffffffff;

void intr_handler(void) {
/* 	static int debug = 0; */
	outb(output, LPT_PORT);
	output = ~output;
/* 	if(debug++ == 1000){conpr("x"); debug = 0;} */
/* 	rtl_printf("i"); */
}

int init_module(void)
{
	request_RTirq(LPT_IRQ, intr_handler);
	outb_p(0x10, LPT_PORT + 2);
	return 0;
} 


void cleanup_module(void)
{
	free_RTirq(LPT_IRQ);
}
