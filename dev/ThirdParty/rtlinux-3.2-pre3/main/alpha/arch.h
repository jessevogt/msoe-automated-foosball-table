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

#ifndef _ALPHA_ARCH_H
#define _ALPHA_ARCH_H
#include <linux/irq.h>
#include <linux/config.h>
#include <arch/constants.h>
#include <rtl_core.h>

#ifndef CONFIG_RTLINUX
#error CONFIG_RTLINUX is not set in the kernel source
#endif

#define RTL_NR_IRQS ACTUAL_NR_IRQS

extern void (*hard_do_IRQ)(unsigned long, struct pt_regs *);
/*
 * this is in arch/alpha/kernel/irq_impl.h so we have our own
 * prototype here -- Cort
 */
extern void (*handle_irq)(unsigned long, struct pt_regs *);
extern void (*handle_ipi)(struct pt_regs *);
extern int last_irq[NR_CPUS];
extern int last_local[NR_CPUS];
extern void (*hard_smp_percpu_timer_interrupt)(struct pt_regs *);
extern void (*smp_percpu_timer_interrupt)(struct pt_regs *);
extern void (*hard_handle_ipi)(struct pt_regs *);

/*
 * Global dispatch functions
 */
#define dispatch_linux_irq(regs,irq) { hard_do_IRQ(irq,regs); }
extern __inline__ void soft_dispatch_global(unsigned int irq)
{
	struct pt_regs regs;
	hard_do_IRQ(irq,&regs);
}

#define ARCH_DEFINED_ENABLE (IPL_MIN)
#define ARCH_DEFINED_DISABLE (IPL_MAX)

extern irq_desc_t rtl_hard_irq_desc[NR_IRQS];

#define rtl_irq_controller_enable(irq) \
({ if (rtl_hard_irq_desc[irq].handler && rtl_hard_irq_desc[irq].handler->enable) \
	rtl_hard_irq_desc[irq].handler->enable(irq); })
#define rtl_irq_controller_disable(irq) \
({ if (rtl_hard_irq_desc[irq].handler && rtl_hard_irq_desc[irq].handler->disable) \
	rtl_hard_irq_desc[irq].handler->disable(irq); })
#define rtl_irq_controller_get_irq(regs) \
({ int i = last_irq[cpu_id]; last_irq[cpu_id] = -1; i; })
#define rtl_irq_controller_ack(irq) \
({ if (rtl_hard_irq_desc[irq].handler && rtl_hard_irq_desc[irq].handler->ack) \
	rtl_hard_irq_desc[irq].handler->ack(irq); })
#define rtl_local_irq_controller_ack() { }

#define LOCAL_TIMER_VECTOR	1
#define LOCAL_IPI_VECTOR	2

#define dispatch_local_linux_irq(regs,pnd) {		\
	if ( pnd == LOCAL_TIMER_VECTOR ) 		\
		hard_smp_percpu_timer_interrupt(regs);	\
	else if ( pnd == LOCAL_IPI_VECTOR) 		\
		hard_handle_ipi(regs); 			\
	else 						\
		printk("dispatch_local_linux_irq(): unknown pnd %d\n", pnd); \
}

#define dispatch_rtl_local_handler(pnd,regs) /* not used */
extern __inline__ void soft_dispatch_local(unsigned int irq)
{
	struct pt_regs regs;
	dispatch_local_linux_irq(&regs,irq);
}

#endif /* _ALPHA_ARCH_H */
