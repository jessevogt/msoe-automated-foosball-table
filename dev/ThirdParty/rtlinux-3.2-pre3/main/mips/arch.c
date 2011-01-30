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

#include <linux/irq.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/irq.h>

#include <rtl_core.h>
#include <rtl_sync.h>
#include "arch.h"

irq_desc_t rtl_hard_irq_desc[NR_IRQS];
int last_irq = -1;
int (*hard_do_IRQ)(int, struct pt_regs *);
void (*mips_hard_sti)(void);
void (*mips_hard_cli)(void);
void (*mips_hard_save_flags_ptr)(unsigned long *);
void (*mips_hard_save_and_cli_ptr)(unsigned long *);
void (*mips_hard_restore_flags)(ulong);

extern int (*do_IRQ)(int, struct pt_regs *);
extern ulong soft_sti;

void mips_ack(unsigned int irq)
{
}

void mips_end(unsigned int irq)
{
	rtl_virt_enable(irq);
}

unsigned int mips_startup(unsigned int irq)
{
	rtl_virt_enable(irq);
	return 0;
}

int
rtl_irq_set_affinity(unsigned int irq, const unsigned long *mask,
		     unsigned long *oldmask)
{
	return -1;
}

hw_irq_controller rtl_fake_pic =
{ " RTLinux PIC  ", mips_startup, rtl_virt_disable, rtl_virt_enable,
  rtl_virt_disable, mips_ack, mips_end
};

int mips_intercept(int irq, struct pt_regs *regs)
{
	if (last_irq != -1)
		printk("mips_intercept: last_irq set to %d current %d\n",
		       last_irq, irq);
	last_irq = irq;
	return rtl_intercept(regs);
}

int arch_takeover(void)
{
	int i;
	void rtl_soft_sti_no_emulation(void);

	__cli();

	/* save and then replace the cli/sti calls */
	mips_hard_sti = __sti;
	mips_hard_cli = __cli;
	mips_hard_save_flags_ptr = __save_flags_ptr;
	mips_hard_save_and_cli_ptr = __save_and_cli_ptr;
	mips_hard_restore_flags = __restore_flags;

	__sti = rtl_soft_sti;
	__cli = rtl_soft_cli;
	__save_flags_ptr = rtl_soft_save_flags;
	__save_and_cli_ptr = rtl_soft_save_and_cli;
	__restore_flags = rtl_soft_restore_flags;

	/* take over the call to do_IRQ */
	hard_do_IRQ = do_IRQ;
	do_IRQ = mips_intercept;

	soft_sti = (ulong)rtl_soft_sti;

	/* copy current irq handlers for safekeeping */
	memcpy(rtl_hard_irq_desc, irq_desc, sizeof(irq_desc_t) * NR_IRQS);
	/* then replace them with our fake one. */
	for (i = 0; i < RTL_NR_IRQS; i++)
		irq_desc[i].handler = &rtl_fake_pic;

	rtl_hard_sti();
	return 0;
}

void arch_giveup(void)
{
	rtl_hard_cli();

	/* restore the cli/sti calls */
	__sti = mips_hard_sti;
	__cli = mips_hard_cli;
	__save_flags_ptr = mips_hard_save_flags_ptr;
	__save_and_cli_ptr = mips_hard_save_and_cli_ptr;
	__restore_flags = mips_hard_restore_flags;


	do_IRQ = hard_do_IRQ;

	soft_sti = 0;

	/* copy irq handlers back */
	memcpy(irq_desc, rtl_hard_irq_desc, sizeof(irq_desc_t) * NR_IRQS);

	__sti();
}

void dispatch_rtl_local_irq(int irq)
{
}

int rtl_free_local_irq(int i, unsigned int cpu)
{
	return 0;
}

int
rtl_request_local_irq(int i, unsigned int (*handler) (struct pt_regs * r),
		      unsigned int cpu)
{
	return 0;
}
