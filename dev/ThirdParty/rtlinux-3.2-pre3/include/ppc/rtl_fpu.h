/*
 * PPC-specific FPU operations for RTLinux
 * 
 * Copyright (C) 1999 FSM Labs (http://www.fsmlabs.com/)
 *  Written by Cort Dougan <cort@fsmlabs.com>
 */

#ifndef __RTL_FPU_H
#define __RTL_FPU_H

struct rtl_fpu_context
{
	double		fpr[32];	/* Complete floating point set */
	unsigned long	fpscr_pad;	/* fpr ... fpscr must be contiguous */
	unsigned long	fpscr;	
	unsigned long	msr;	
};

typedef struct rtl_fpu_context RTL_FPU_CONTEXT;

#endif /* __RTL_FPU_H */
