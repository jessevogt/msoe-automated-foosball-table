/*
 * RTLinux PPC-specific header.
 *
 * Copyright (C) 1999 Cort Dougan <cort@fsmlabs.com>
 */
#ifndef __PPC_CONSTANTS_H__
#define __PPC_CONSTANTS_H__

#include <asm/irq.h>

#define __LOCAL_IRQS__
#define MACHDEPREGS struct pt_regs *
typedef int intercept_t;  /* intercept returns code determining whether
				to use linux ret_from intercept */
				
#define MACHDEPREGS_PTR(x) (x)
#define IRQ_MAX_COUNT NR_IRQS

/* only 1 local intr on the PPC right now - timer -- Cort */
#define MACHDEPREGS_TO_PND(x)		(0)
#define VECTOR_TO_IRQ(vector) 		(0)
#define VECTOR_TO_LOCAL_PND(vector)	(0)
#define LOCAL_PND_TO_VECTOR(x)		(0)

#define RETURN_FROM_INTERRUPT return 0
#define RETURN_FROM_INTERRUPT_LINUX return 1
#define RETURN_FROM_LOCAL return 0
#define RETURN_FROM_LOCAL_LINUX return 1

#define rtl_getcpuid() hard_smp_processor_id()
#endif /* __PPC_CONSTANTS_H__ */
