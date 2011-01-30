/*
 * Copyright (C) 1999 FSM Labs (http://www.fsmlabs.com/)
 *  Written by Cort Dougan <cort@fsmlabs.com>
 *
 */
#ifndef __ARCH_RTL_SYNC__
#define __ARCH_RTL_SYNC__

#include <linux/irq.h>
#include <asm/system.h>
#include <asm/hw_irq.h>

typedef unsigned long rtl_irqstate_t;

#define __rtl_hard_cli() ((void) swpipl(IPL_MAX))
#define __rtl_hard_sti() ((void) swpipl(/*IPL_MIN*/0))
#define __rtl_hard_save_flags(x) ((x) = rdps())
#define __rtl_hard_restore_flags(x) ((void) swpipl(x))
#define __rtl_hard_savef_and_cli(x) \
	do { rtl_hard_save_flags(x); rtl_hard_cli(); } while(0);
#endif
