#include <linux/module.h>
#include <linux/sched.h>
#include <linux/rtl.h>
#include <asm/io.h>
#include <linux/cons.h>
#include "common.h"

static int output;

void intr_handler(int irq, void *id, struct pt_regs *regs) {
	outb(output, LPT_PORT);
	output = !output;
}

int init_module(void)
{
	request_irq(LPT_IRQ, intr_handler, SA_INTERRUPT, "lpt", NULL);
	outb_p(0x10, LPT_PORT + 2);
	output = 0;
	return 0;
} 


void cleanup_module(void)
{
	free_irq(LPT_IRQ, NULL);
}

