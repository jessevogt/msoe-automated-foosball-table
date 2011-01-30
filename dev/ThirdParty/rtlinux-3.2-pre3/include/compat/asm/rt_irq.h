/*
 * RTLinux v1 API compatibility layer
 * Written by Michael Barabanov
 * Copyright (C) 1999-2000Finite State Machine Labs Inc. 
 *
 */
#ifndef __RTL_RT_IRQ_H__
#define __RTL_RT_IRQ_H__
#include <asm/ptrace.h>
#include <rtl_time.h>
#include <rtl_core.h>
#include <rtl_sync.h>

#define r_sti() rtl_allow_interrupts()
#define r_cli() rtl_stop_interrupts()

#define r_save_flags(x) rtl_hard_save_flags(x)
#define r_restore_flags(x) rtl_hard_restore_flags(x)


extern int request_RTirq(unsigned   int   irq,   void (*handler)(void));

extern inline int free_RTirq(unsigned int irq)
{
	return rtl_free_global_irq (irq);
}


#endif
