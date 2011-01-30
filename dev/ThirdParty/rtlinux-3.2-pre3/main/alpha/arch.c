/*
 * (C) Finite State Machine Labs Inc. 2000 <business@fsmlabs.com>
 *
 * Released under the terms of GPL 2.
 * Open RTLinux makes use of a patented process described in
 * US Patent 5,995,745. Use of this process is governed
 * by the Open RTLinux Patent License which can be obtained from
 * www.fsmlabs.com/PATENT or by sending email to
 * licensequestions@fsmlabs.com
 */

#include <asm/uaccess.h>
#include <asm/system.h>
#include <linux/irq.h>
#include <asm/irq.h>
#include <asm/machvec.h>

#include "arch.h"
#include <rtl_core.h>
#include <rtl_sync.h>

extern unsigned long linux_soft_sti;

irq_desc_t rtl_hard_irq_desc[NR_IRQS];
void (*hard_do_IRQ)(unsigned long, struct pt_regs *);
void (*hard_smp_percpu_timer_interrupt)(struct pt_regs *);
void rtl_local_intercept(struct pt_regs *regs);
int last_irq[NR_CPUS] = {-1, };
int last_local[NR_CPUS] = {-1, };
unsigned long pad, pad1;
void (*hard_handle_ipi)(struct pt_regs *);
void rtl_soft_sti_no_emulation(void);

void alpha_ack(unsigned int irq)
{
}

void alpha_end(unsigned int irq)
{
	rtl_virt_enable(irq);
}

unsigned int alpha_startup(unsigned int irq)
{
	rtl_virt_enable(irq);
	return 0;
}

int rtl_irq_set_affinity (unsigned int irq, const unsigned long *mask, unsigned long *oldmask)
{
	return -1;
}

hw_irq_controller rtl_fake_pic =
{ " RTLinux PIC  ", alpha_startup, rtl_virt_disable, rtl_virt_enable,
  rtl_virt_disable, alpha_ack, alpha_end };

unsigned long alpha_soft_rdps(void)
{
	unsigned long x;
	rtl_soft_save_flags(&x);
	return (rdps()&~7) | (x&7);
}

unsigned long alpha_soft_swpipl(unsigned long ipl)
{
	unsigned long prev = alpha_soft_rdps()&7;
	extern unsigned long last_cli[NR_CPUS];
	unsigned int cpu_id = rtl_getcpuid();
	ipl &= 7;
	
	/* special case since IPL_MIN can be a variable */
	if ( ipl == IPL_MIN )
	{
		rtl_soft_sti();
		return prev;
	}
	switch ( ipl )
	{
	case IPL_MAX:
		{
			unsigned long flags;
			rtl_soft_save_flags(&flags);
			rtl_soft_cli();
			if ( !(flags & 7) )
				last_cli[cpu_id] = (ulong)__builtin_return_address(0);
		}
		break;
	case 0:
		rtl_soft_sti();
		break;
	default:
		rtl_printf("alpha_soft_setipl: unknown IPL %x\n", ipl );
		break;
	}
	return prev;
}

#ifdef CONFIG_SMP
static void alpha_handle_ipi(struct pt_regs *regs)
{
	unsigned int cpu_id = rtl_getcpuid();
	/*
	 * alpha_handle_ipi cannot run with concurrently with itself because
	 * of the last_irq hack.  the cli protects us from higher
	 * priority interrupts coming in. -- Cort
	 */
	rtl_hard_cli();
	if ( last_local[cpu_id] != -1 )
		printk("alpha_intercept: last_local set to %d\n",
		       last_local[cpu_id]);
	last_local[cpu_id] = LOCAL_IPI_VECTOR;
	rtl_local_intercept(regs);
}

static void alpha_handle_percpu_timer(struct pt_regs *regs)
{
	unsigned int cpu_id = rtl_getcpuid();

	/*
	 * alpha_intercept cannot be re-entered because of the last_irq
	 * hack.  the cli protects us from higher priority interrupts
	 * coming in. -- Cort
	 */
	rtl_hard_cli();
	if ( last_local[cpu_id] != -1 )
		printk("alpha_intercept: last_local set to %d on cpu %d",
		       last_local[cpu_id], cpu_id);
	last_local[cpu_id] = LOCAL_TIMER_VECTOR;
	rtl_local_intercept(regs);
}
#endif /* CONFIG_SMP */

static void alpha_intercept(unsigned long irq, struct pt_regs *regs)
{
	unsigned int cpu_id = rtl_getcpuid();
	/*
	 * alpha_intercept cannot be re-entered because of the last_irq
	 * hack.  the cli protects us from higher priority interrupts
	 * coming in. -- Cort
	 */
	rtl_hard_cli();
	if ( last_irq[cpu_id] != -1 )
		printk("alpha_intercept: last_irq set to %d current %ld\n",
		       last_irq[cpu_id], irq);
	last_irq[cpu_id] = irq;
	rtl_intercept(regs);
}

struct {
	atomic_t waiting;
	atomic_t done;
} sync_data = {{0},{0}};

void sync_takeover(void *unused)
{
	int i;
	rtl_hard_cli();
	atomic_inc(&sync_data.waiting);
	i = 0;
	while ( !atomic_read(&sync_data.done) && (i < 1000000000) )
	{
		i++;
		if (i == 1000000000)
			printk("timed out on sync_data.done\n");
	}
	rtl_hard_sti();
}

int arch_takeover(void)
{
	int i;
	void rtl_soft_sti_no_emulation(void);
	
#ifdef CONFIG_SMP	
	int timeout, cpus = smp_num_cpus - 1;
#endif /* CONFIG_SMP */

	if ( IPL_MIN != 0 )
	{
		printk("Your alpha system is not supported.\n");
		return -1;
	}
	
	__cli();

#ifdef CONFIG_SMP	
	smp_call_function( sync_takeover, 0, 0 /*atomic */,0 /*don't wait*/ );
	/* everyone else is now starting to exec sync_function */
	timeout = jiffies + HZ;
	while ((atomic_read(&sync_data.waiting) != cpus)
			&& time_before(jiffies, timeout));
	if(atomic_read(&sync_data.waiting) != cpus)
	{
	       printk("rtl_smp_synchronize timed out\n");
       	       return -1;
	}
	hard_smp_percpu_timer_interrupt = smp_percpu_timer_interrupt;
	smp_percpu_timer_interrupt = alpha_handle_percpu_timer;

	hard_handle_ipi = handle_ipi;
	handle_ipi = alpha_handle_ipi;
	for ( i = 0; i < smp_num_cpus; i++ )
		last_local[i] = last_irq[i] = -1;
#endif /* CONFIG_SMP */

	memcpy(rtl_hard_irq_desc, irq_desc, sizeof(irq_desc_t)*NR_IRQS);
	pad = (ulong)__rdps;
	pad1 = (ulong)__swpipl;
	__rdps = alpha_soft_rdps;
	__swpipl = alpha_soft_swpipl;
 
	for ( i = 0 ; i < NR_IRQS; i++ )
		if ( irq_desc[i].handler )
			irq_desc[i].handler = &rtl_fake_pic;

	hard_do_IRQ = handle_irq;
	handle_irq = alpha_intercept;
	linux_soft_sti = (ulong)rtl_soft_sti_no_emulation;
	rtl_hard_sti();
#ifdef CONFIG_SMP
	atomic_set( &sync_data.done, 1);
#endif /* CONFIG_SMP */	
	return 0;
}

void arch_giveup(void)
{
	rtl_hard_cli();
	(ulong)__rdps = pad;
	(ulong)__swpipl = pad1;
	memcpy(irq_desc, rtl_hard_irq_desc, sizeof(irq_desc_t)*NR_IRQS);
	linux_soft_sti = 0;
	handle_irq = hard_do_IRQ;
#ifdef CONFIG_SMP	
	handle_ipi = hard_handle_ipi;
	smp_percpu_timer_interrupt = hard_smp_percpu_timer_interrupt;
#endif /* CONFIG_SMP */	
	__sti();
}

int rtl_free_local_irq(int i, unsigned int cpu)
{
	return 0;
}

int rtl_request_local_irq(int i, unsigned int (*handler)(struct pt_regs *r),
			  unsigned int cpu)
{
	return 0;
}
