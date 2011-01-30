/*
 * (C) Finite State Machine Labs Inc. 1999 <business@fsmlabs.com>
 *
 * Released under the terms of GPL 2.
 * Open RTLinux makes use of a patented process described in
 * US Patent 5,995,745. Use of this process is governed
 * by the Open RTLinux Patent License which can be obtained from
 * www.fsmlabs.com/PATENT or by sending email to
 * licensequestions@fsmlabs.com
 */

#ifndef _PPC_ARCH_H
#define _PPC_ARCH_H

#include <linux/irq.h>
#include <linux/autoconf.h>
#include <asm/hardirq.h>

#include <arch/constants.h>
#include <rtl_core.h>

#ifdef CONFIG_MOL
#error You cannot run RTLinux with MOL (Mac-on-Linux) enabled - turn it off in the Linux config
#endif

#define RTL_NR_IRQS NR_IRQS

#define soft_local_disabled_irq(irq) (0) /* cannot disable timer */

extern unsigned int (*timer_handler)(struct pt_regs *r);
void rtl_hard_pic_end(unsigned int irq_nr);

/*
 * Global dispatch functions
 */
void ppc_irq_dispatch_handler(struct pt_regs *regs, int irq);

extern __inline__ void dispatch_linux_irq(struct pt_regs *regs, int irq)
{
	hardirq_enter(smp_processor_id());
	ppc_irq_dispatch_handler(regs,irq);
	rtl_hard_pic_end(irq);
	hardirq_exit(smp_processor_id());
}

extern __inline__ void soft_dispatch_global(unsigned int irq)
{
	struct pt_regs fake_regs;
	hardirq_enter( smp_processor_id() );
	ppc_irq_dispatch_handler(&fake_regs, irq);
	rtl_hard_pic_end(irq);
	hardirq_exit( smp_processor_id() );
}

/*
 * Local irq functions
 */ 
#define dispatch_rtl_local_handler(pnd,regs) timer_handler(regs)
#define dispatch_local_linux_irq(regs,pnd) timer_interrupt(regs)
extern __inline__ void soft_dispatch_local(unsigned int irq)
{
	struct pt_regs regs;
	dispatch_local_linux_irq(&regs, 0);
}

#define ARCH_DEFINED_ENABLE (MSR_EE)
#define ARCH_DEFINED_DISABLE 0

extern irq_desc_t rtl_hard_irq_desc[NR_IRQS];
extern struct int_control_struct rtl_hard_int_control;
extern int (*hard_get_irq)(struct pt_regs *);

#define rtl_irq_controller_enable(irq) \
({ if (rtl_hard_irq_desc[irq].handler && rtl_hard_irq_desc[irq].handler->enable) \
	rtl_hard_irq_desc[irq].handler->enable(irq); })
#define rtl_irq_controller_disable(irq) \
({ if (rtl_hard_irq_desc[irq].handler && rtl_hard_irq_desc[irq].handler->disable) \
	rtl_hard_irq_desc[irq].handler->disable(irq); })
#define rtl_irq_controller_get_irq(regs) hard_get_irq(regs)
#define rtl_irq_controller_ack(irq) \
({ if (rtl_hard_irq_desc[irq].handler && rtl_hard_irq_desc[irq].handler->ack) \
	rtl_hard_irq_desc[irq].handler->ack(irq); })
#define rtl_local_irq_controller_ack() { }

#endif /* _PPC_ARCH_H */
