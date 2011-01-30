/*
 * RTLinux Alpha-specific header.
 *
 * Copyright (C) 2000 Cort Dougan <cort@fsmlabs.com>
 */
#ifndef __ALPHA_CONSTANTS_H__
#define __ALPHA_CONSTANTS_H__

#include <asm/irq.h>

#define __LOCAL_IRQS__
#define MACHDEPREGS struct pt_regs *
typedef void intercept_t;  /* intercept returns code determining whether
			      to use linux ret_from intercept */

#define MACHDEPREGS_PTR(x) (x)
#define IRQ_MAX_COUNT NR_IRQS

/*
 * This really needs to be redone soon. -- Cort
 */
static __inline__ void rtl_return_from_interrupt(void)
{
	unsigned long i;
	extern void ret_from_sys_call(void);
	extern void restore_all_rtl(void);
	i = (ulong)&i;
	i += 0x30;
	if ( *(ulong *)i == (ulong)ret_from_sys_call )
	{
		*(ulong *)i = (ulong)restore_all_rtl;
		return;
	}
	i += 0x10;
	if ( *(ulong *)i == (ulong)ret_from_sys_call )
	{
		*(ulong *)i = (ulong)restore_all_rtl;
		return;
	}
	
	for ( i = (ulong)&i; i <= ((ulong)&i) + sizeof(ulong)*1024;
	      i += sizeof(ulong) )
	{
		if ( *(ulong *)i == (ulong)ret_from_sys_call )
		{
			/*printk("offset %lx\n", i-((ulong)&i) );*/
			*(ulong *)i = (ulong)restore_all_rtl;
			return;
		}
	}
	return;
}

#define RETURN_FROM_INTERRUPT rtl_return_from_interrupt()
#define RETURN_FROM_INTERRUPT_LINUX return
/*
 * Local interrupts (specifically the timer) on Alpha are
 * done with a call to the local function, then sometimes a
 * call to the global interrupt handler routine.  This will
 * always fail since the RTLinux local intercept leaves
 * interrupts soft disabled.  So, we soft enable here
 * so after the return we end up with soft enabled when
 * calling the global handler from the Linux routine.
 *
 * We also have a special case for the RETURN_FROM_LOCAL, when
 * we want to return directly to Linux since the local intr is
 * disabled.
 * -- Cort
 */
#define RETURN_FROM_LOCAL { \
	if ( pnd == LOCAL_IPI_VECTOR) \
		rtl_return_from_interrupt(); \
	else \
		return; \
}
#define RETURN_FROM_LOCAL_LINUX {rtl_soft_sti(); return;}

#define MACHDEPREGS_TO_PND(x)		({ ulong i = last_local[cpu_id]; last_local[cpu_id] = -1; i;})
#define VECTOR_TO_IRQ(vector) 		(vector)
#define VECTOR_TO_LOCAL_PND(vector)	(vector)
#define LOCAL_PND_TO_VECTOR(x)		(x)

#define rtl_getcpuid() hard_smp_processor_id()
#endif /* __ALPHA_CONSTANTS_H__ */
