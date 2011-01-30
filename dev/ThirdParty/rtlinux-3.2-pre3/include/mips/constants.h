/*
 * RTLinux MIPS-specific header.
 *
 * Copyright (C) 2000 Cort Dougan <cort@fsmlabs.com>
 */
#ifndef __MIPS_CONSTANTS_H__
#define __MIPS_CONSTANTS_H__

#include <linux/kernel.h>
#include <asm/irq.h>

#define MACHDEPREGS struct pt_regs *
#define rtl_irq_get_return_addr(regs) ((void *)((regs)->cp0_epc))
typedef int intercept_t;	/* intercept returns code determining whether
				   to use linux ret_from intercept */

#define MACHDEPREGS_PTR(x) (x)
#define IRQ_MAX_COUNT NR_IRQS

#define RETURN_FROM_INTERRUPT return 1
#define RETURN_FROM_INTERRUPT_LINUX return 0
#define RETURN_FROM_LOCAL return 1
#define RETURN_FROM_LOCAL_LINUX return 0

#define rtl_getcpuid() hard_smp_processor_id()
#endif				/* __MIPS_CONSTANTS_H__ */
