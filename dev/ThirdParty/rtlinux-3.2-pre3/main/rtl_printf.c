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

#include <linux/kernel.h>
#include <rtl_conf.h>
#include <rtl_core.h>
#include <rtl_sync.h>
#include <rtl_printf.h>
#include <stdarg.h>
#include <asm/system.h>

#ifdef CONFIG_RTL_SLOW_CONSOLE
static int rtl_printf_irq = 0;
void rtl_printf_handler(int irq,void *ignore,struct pt_regs *ignoreregs);
static spinlock_t rtl_printf_handler_lock = SPIN_LOCK_UNLOCKED;
#endif


/* This moves some junk off stack and also allows a compile time
   option to use a Linux driver so that rtl_printf merely passes
   data to Linux printk. The idea here was from David
   Schleef and the code was introduced in the core module to 
   make slow serial consoles work with RTLinux
   */
#define MAX_PRINTKBUF 2000
static char initial_printkbuf [MAX_PRINTKBUF]; /* need to protect in_printkbuf from overflowing */
static char in_printkbuf[MAX_PRINTKBUF]; /* please don't put this on my stack*/
static char *printkptr = &in_printkbuf[0];
static spinlock_t rtl_cprintf_lock = SPIN_LOCK_UNLOCKED;
int rtl_printf(const char * fmt, ...)
{
	rtl_irqstate_t flags;
	int i;
	va_list args;

	rtl_no_interrupts(flags);
	rtl_spin_lock(&rtl_cprintf_lock);

	va_start(args, fmt);
	/* dangerous. Don't rtl_printf long strings */
	i=vsprintf(initial_printkbuf,fmt,args);
	va_end(args);

#ifndef CONFIG_RTL_SLOW_CONSOLE
	conpr(initial_printkbuf);
#else
	/* perhaps we should discard old data instead */
	if (i > MAX_PRINTKBUF - (printkptr - in_printkbuf) - 30) {
		i = MAX_PRINTKBUF - (printkptr - in_printkbuf) - 30;
	}
	if (i <= 0) {
		rtl_spin_unlock(&rtl_cprintf_lock);
		rtl_restore_interrupts(flags);
		return 0;
	}
	memcpy (printkptr, initial_printkbuf, i);
	printkptr += i;
	*printkptr = 0;
/*	if ((printkptr - in_printkbuf) > MAX_PRINTKBUF/2)
	{
		printkptr = &in_printkbuf[0];
		i= vsprintf(printkptr,"PRINTK OVERFLOW\n",0);
		printkptr +=i;
	} */
	if (rtl_printf_irq) {
		rtl_global_pend_irq(rtl_printf_irq);
	}
#endif
	rtl_spin_unlock(&rtl_cprintf_lock);
	rtl_restore_interrupts(flags);

	return i;
}
#ifdef CONFIG_RTL_SLOW_CONSOLE
static char out_printkbuf[MAX_PRINTKBUF]; /* the buffer printk actually prints from */
void rtl_printf_handler(int irq,void *ignore,struct pt_regs *ignoreregs)
{
	rtl_irqstate_t flags;
	long linux_flags;
	__save_flags(linux_flags);
	__cli();
	spin_lock(&rtl_printf_handler_lock);

	rtl_no_interrupts(flags);
	rtl_spin_lock(&rtl_cprintf_lock);

	memcpy (out_printkbuf, in_printkbuf, printkptr - in_printkbuf + 1);
	printkptr = &in_printkbuf[0];
	*printkptr = 0;

	rtl_spin_unlock (&rtl_cprintf_lock);
	rtl_restore_interrupts (flags);

	printk("%s", out_printkbuf);
	spin_unlock(&rtl_printf_handler_lock);
	__restore_flags(linux_flags);
}

#endif

int rtl_printf_init(void)
{
#ifdef CONFIG_RTL_SLOW_CONSOLE
	rtl_printf_irq = rtl_get_soft_irq(rtl_printf_handler,"RTLinux printf");
	if (rtl_printf_irq < 0) {
		printk("RTL: couldn't get a soft irq for rtl_printf\n");
		return -1;
	}
#endif
	return 0;
}

void rtl_printf_cleanup(void)
{
#ifdef CONFIG_RTL_SLOW_CONSOLE
	if (rtl_printf_irq) {
		rtl_free_soft_irq(rtl_printf_irq);
	}
#endif
}

/* goes directly to console_drivers */
int rtl_cprintf(const char * fmt, ...)
{
	rtl_irqstate_t flags;
	int i;
	va_list args;

	rtl_hard_savef_and_cli(flags);
	rtl_spin_lock(&rtl_cprintf_lock);

	va_start(args, fmt);
	/* dangerous. Don't rtl_printk long strings */
	i=vsprintf(printkptr,fmt,args);
	va_end(args);
	conpr(in_printkbuf);
	rtl_spin_unlock(&rtl_cprintf_lock);
	rtl_hard_restore_flags(flags);

	return i;
}

