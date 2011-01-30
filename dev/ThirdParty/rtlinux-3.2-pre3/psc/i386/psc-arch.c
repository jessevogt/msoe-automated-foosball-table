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
 * based on debugger/i386/rtl-stub.c
 *
 * June 2003, Nils Hasler <nils@penai.de>
 *    initial release
 */

#ifdef PSC_EXCEPTION

//#include <asm/vm86.h>
#include <linux/signal.h>

//#define __NO_VERSION__
//#include <linux/module.h>
//#include <rtl_sched.h>
//#include <psc.h>

#define compute_signal()  computeSignal(vector)

static struct hard_trap_info
{
	unsigned int tt;		/* Trap type code for i386 */
	unsigned char signo;		/* Signal that we map this trap into */
} hard_trap_info[] = {
	{ 0,  SIGFPE },
	{ 1, SIGTRAP },
	{ 2, SIGSEGV },
	{ 3, SIGTRAP },
	{ 4, SIGSEGV },
	{ 5, SIGSEGV },
	{ 6, SIGILL },
	{ 7, SIGSEGV },
	{ 8, SIGSEGV },
	{ 9, SIGFPE },
	{ 10, SIGSEGV },
	{ 11, SIGBUS },
	{ 12, SIGBUS },
	{ 13, SIGSEGV },
	{ 17, SIGSEGV },
	{ 18, SIGSEGV },
	{ 19, SIGSEGV },
};

int computeSignal(unsigned int tt)
{
	int i;

	for (i = 0; i < sizeof(hard_trap_info)/sizeof(hard_trap_info[0]); i++) {
		if (hard_trap_info[i].tt == tt) {
			return hard_trap_info[i].signo;
		}
	}

	return SIGHUP;         /* default for things we don't know about */
}

#endif /* PSC_EXCEPTION */
