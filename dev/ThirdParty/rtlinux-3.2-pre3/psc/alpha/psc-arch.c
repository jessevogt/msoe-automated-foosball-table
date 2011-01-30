/*
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 */

/*
 * based on debugger/alpha/rtl-stub.c
 *
 * June 2003, Nils Hasler <nils@penai.de>
 *    initial release
 */

#ifdef PSC_EXCEPTION

/*#include <asm/system.h>
#include <asm/ptrace.h>		/* for linux pt_regs struct */
/*#include <asm/reg.h>		/* for EF_* reg num defines */
#include <asm/gentrap.h>	/* for GEN_* trap num defines */
/*#include <linux/smp.h>
#include <linux/smp_lock.h>
#include <linux/kernel.h>
#include <linux/mm.h>*/
#include <linux/signal.h>

/* RTLinux support */
//#define __NO_VERSION__
//#include <linux/module.h>
//#include <rtl_sched.h>
//#include <psc.h>
#include <rtl_ex.c>

#define compute_signal()  computeSignal(vector, regs)

/* we don't have a hard_trap_info struct and we have to use this gigantic
 * switch statement because of the special case of generic traps.  This also
 * means that we have to pass across the registers so that we can get the
 * value from r16 to determine which generic trap it is. */
static int computeSignal(unsigned int tt, struct pt_regs *regs)
{
	switch (tt) {
	case 0:		/* breakpoint */
	case 1:		/* bugcheck */
		return SIGTRAP;

	case 2:		/* gentrap */
		switch ((long) regs->r16) {
		case GEN_INTOVF:
		case GEN_INTDIV:
		case GEN_FLTOVF:
		case GEN_FLTDIV:
		case GEN_FLTUND:
		case GEN_FLTINV:
		case GEN_FLTINE:
		case GEN_ROPRAND:
			return SIGFPE;

		case GEN_DECOVF:
		case GEN_DECDIV:
		case GEN_DECINV:
		case GEN_ASSERTERR:
		case GEN_NULPTRERR:
		case GEN_STKOVF:
		case GEN_STRLENERR:
		case GEN_SUBSTRERR:
		case GEN_RANGERR:
		case GEN_SUBRNG:
		case GEN_SUBRNG1:
		case GEN_SUBRNG2:
		case GEN_SUBRNG3:
		case GEN_SUBRNG4:
		case GEN_SUBRNG5:
		case GEN_SUBRNG6:
		case GEN_SUBRNG7:
			return SIGTRAP;
		}		/* switch ((long) regs->r16) */

	case 3:		/* FEN fault */
		return SIGILL;

	case 4:		/* opDEC */
		return SIGILL;
	}			/* switch (tt) */

	/* don't know what signal to return?  SIGHUP to the rescue! */
	return SIGHUP;
}

#endif /* PSC_EXCEPTION */
