/*
 * Interrupt handling in Real-Time. react to an RT-interrupt by toggling all
 * bits on the parallel port
 *
 * Sotlen from Michael Barabanov by Der Herr Hofrat,
 * (C) FSMLabs  2001. der.herr@hofr.at
 * Released under the GNU GENERAL PUBLIC LICENSE Version 2, June 1991
 * Any use of this code must include this notice.
 */
#include <rtl_core.h>
#include <rtl_time.h>
#include <asm/io.h>
#include <rtl.h>
#include "common.h"

unsigned int 
intr_handler(unsigned int irq, struct pt_regs *regs)
{
	outb(0x0ff,LPT); /* respond to an interrupt by toggling the databits*/
	return 0;
}

int 
init_module(void)
{
	rtl_request_irq (LPT_IRQ, intr_handler);
	rtl_hard_enable_irq (LPT_IRQ);
	return 0;
} 

void 
cleanup_module(void)
{
	rtl_free_irq(LPT_IRQ);
}
