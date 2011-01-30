/*
 * Copyright (C) 1999-2001 FSM Labs (http://www.fsmlabs.com/)
 *  Written by Cort Dougan <cort@fsmlabs.com>
 *
 */
#ifndef __ARCH_RTL_SYNC__
#define __ARCH_RTL_SYNC__

#include <linux/version.h>

#include <asm/ptrace.h>
#include <asm/atomic.h>
#include <asm/hw_irq.h>

typedef unsigned long rtl_irqstate_t;

#define __rtl_hard_cli() __asm__ __volatile__ ( 	\
	"mfmsr	0\n\t"			\
	"rlwinm	0,0,0,17,15\n\t"	\
	"sync\n\t"			\
	"mtmsr	0\n\t"			\
	"sync\n\t" : /* output */ : /* input */ : "r0" )

#define __rtl_hard_sti() __asm__ __volatile__ ( 	\
	"mfmsr	3\n\t"			\
	"ori	3,3,%0\n\t"		\
	"sync\n\t"			\
	"mtmsr	3\n\t"			\
	"sync\n\t" : /* output */ : "i" (MSR_EE) : "r3" )

#define __rtl_hard_save_flags(x) __asm__ __volatile__ ( "mfmsr	%0" : "=r" (x) )

#define __rtl_hard_restore_flags(x) __asm__ __volatile__  ( \
	"mfmsr 	4\n\t"			\
	"rlwimi	%0,4,0,17,15\n\t"	\
	/*"cmpw	0,%0,4\n\t"*/		\
	/*"beq	10f\n\t"*/		\
	"1:sync\n\t"			\
	"mtmsr	%0\n\t"			\
	"sync\n10:\t" : /* output */ : "r" (x) : "r0", "r4")

#define __rtl_hard_savef_and_cli(x) __asm__ __volatile__ ( \
	"mfmsr	%0\n\t"			\
	"rlwinm	0,%0,0,17,15\n\t"	\
	"sync\n\t"			\
	"mtmsr	0\n\t"			\
	"sync\n\t" : "=r" (x) : /* input */ : "r0" )

#endif
