/*
 * (C) Finite State Machine Labs Inc. 2000 business@fsmlabs.com
 *
 * Released under the terms of GPL 2.
 * Open RTLinux makes use of a patented process described in
 * US Patent 5,995,745. Use of this process is governed
 * by the Open RTLinux Patent License which can be obtained from
 * www.fsmlabs.com/PATENT or by sending email to
 * licensequestions@fsmlabs.com
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <rtl_core.h>
#include <rtl_sync.h>
#include <asm/pgtable.h>
#include <asm/ptrace.h>

#define NR_EXCEPT	6

extern unsigned long rtlinux_do_entIF;
extern unsigned long rtlinux_do_entArith;
extern unsigned long rtlinux_do_page_fault;
extern unsigned long rtlinux_do_entDbg;
extern unsigned long rtlinux_do_entUna;
extern unsigned long rtlinux_sys_call_table;

extern unsigned search_exception_table(unsigned long addr);
extern int rtl_debug_exception(int vector, struct pt_regs *regs);
int (*rtl_intercepter) (int vector, struct pt_regs * regs) = 0;

typedef void (*rtl_trap_handler_t) (struct pt_regs *, long, long);

extern void do_entDbg(unsigned long type, unsigned long a1,
		      unsigned long a2, unsigned long a3, unsigned long a4,
		      unsigned long a5, struct pt_regs regs);

asmlinkage int doentDbg(unsigned long type, unsigned long a1,
			unsigned long a2, unsigned long a3,
			unsigned long a4, unsigned long a5,
			struct pt_regs regs)
{
	return rtl_debug_exception(1, &regs);
}

extern void do_page_fault(unsigned long address, unsigned long mmcsr,
			  long cause, struct pt_regs *regs);

asmlinkage int dopagefault(unsigned long address, unsigned long mmcsr,
			   long cause, struct pt_regs *regs)
{
	if ((regs->pc < PAGE_OFFSET) && !(rtl_is_psc_active()))
		return 0;
	else if (search_exception_table(regs->pc) != 0)
		return 0;
	else
		return rtl_debug_exception(6, regs);
}

extern void do_entArith(unsigned long summary, unsigned long write_mask,
			unsigned long a2, unsigned long a3,
			unsigned long a4, unsigned long a5,
			struct pt_regs regs);

asmlinkage int doArith(unsigned long summary, unsigned long write_mask,
		       unsigned long a2, unsigned long a3,
		       unsigned long a4, unsigned long a5,
		       struct pt_regs regs)
{
	return rtl_debug_exception(7, &regs);
}

extern void do_entIF(unsigned long type, unsigned long a1,
		     unsigned long a2, unsigned long a3, unsigned long a4,
		     unsigned long a5, struct pt_regs regs);

asmlinkage int doIF(unsigned long type, unsigned long a1,
		    unsigned long a2, unsigned long a3, unsigned long a4,
		    unsigned long a5, struct pt_regs regs)
{
	/* i don't know why yet, but apparently the PC and PS get switched
	 * somewhere before we get here.  This switches them back. -Nathan */
	if ((regs.pc == 0) && (regs.ps != 0)) {
		regs.pc = regs.ps;
		regs.ps = 0;
	}
	return rtl_debug_exception(0, &regs);
}

struct allregs {
	unsigned long regs[32];
	unsigned long ps, pc, gp, a0, a1, a2;
};

extern void do_entUna(void *va, unsigned long opcode, unsigned long reg,
		      unsigned long a3, unsigned long a4, unsigned long a5,
		      struct allregs regs);

asmlinkage int doentUna(void *va, unsigned long opcode, unsigned long reg,
			unsigned long a3, unsigned long a4,
			unsigned long a5, struct allregs regs)
{
	/* we have to convert the allregs struct to a pt_regs struct.  How
	 * annoying. -Nathan */
	struct pt_regs real_regs;
	real_regs.pc = regs.pc;
	real_regs.ps = regs.ps;
	real_regs.gp = regs.gp;
	real_regs.trap_a0 = regs.a0;
	real_regs.trap_a1 = regs.a1;
	real_regs.trap_a2 = regs.a2;
	real_regs.r0 = regs.regs[0];
	real_regs.r1 = regs.regs[1];
	real_regs.r2 = regs.regs[2];
	real_regs.r3 = regs.regs[3];
	real_regs.r4 = regs.regs[4];
	real_regs.r5 = regs.regs[5];
	real_regs.r6 = regs.regs[6];
	real_regs.r7 = regs.regs[7];
	real_regs.r8 = regs.regs[8];
	real_regs.r16 = regs.regs[16];
	real_regs.r17 = regs.regs[17];
	real_regs.r18 = regs.regs[18];
	real_regs.r19 = regs.regs[19];
	real_regs.r20 = regs.regs[20];
	real_regs.r21 = regs.regs[21];
	real_regs.r22 = regs.regs[22];
	real_regs.r23 = regs.regs[23];
	real_regs.r24 = regs.regs[24];
	real_regs.r25 = regs.regs[25];
	real_regs.r26 = regs.regs[26];
	real_regs.r27 = regs.regs[27];
	real_regs.r28 = regs.regs[28];
	return rtl_debug_exception(5, &real_regs);
}

extern void sys_call_table(void);

asmlinkage void syscalltable(void)
{
	printk("got system call\n");
	return;
}

int setup_intercept(void)
{
	rtl_irqstate_t flags;

	rtl_no_interrupts(flags);

	rtlinux_do_page_fault = (unsigned long) dopagefault;
	rtlinux_do_entIF = (unsigned long) doIF;
	rtlinux_do_entArith = (unsigned long) doArith;
	rtlinux_do_entDbg = (unsigned long) doentDbg;
	rtlinux_do_entUna = (unsigned long) doentUna;
#if 0
	rtlinux_sys_call_table = (unsigned long) syscalltable;
#endif
	rtl_restore_interrupts(flags);
	return 0;
}

void restore_intercept(void)
{
	rtl_irqstate_t flags;

	rtl_no_interrupts(flags);

	rtlinux_do_entIF = (unsigned long) 0;
	rtlinux_do_entArith = (unsigned long) 0;
	rtlinux_do_page_fault = (unsigned long) 0;
	rtlinux_do_entDbg = (unsigned long) 0;
	rtlinux_do_entUna = (unsigned long) 0;
	rtlinux_sys_call_table = (unsigned long) 0;

	rtl_restore_interrupts(flags);
}

int rtl_request_traps(int (*rtl_exception_intercept)
		       (int vector, struct pt_regs * regs))
{
	if (rtl_exception_intercept) {
		if (rtl_intercepter) {
			return -1;
		} else {
			rtl_intercepter = rtl_exception_intercept;
			setup_intercept();
			return 0;
		}
	} else {
		if (!rtl_intercepter) {
			return -1;
		} else {
			rtl_intercepter = rtl_exception_intercept;
			restore_intercept();
			return 0;
		}
	}
}
