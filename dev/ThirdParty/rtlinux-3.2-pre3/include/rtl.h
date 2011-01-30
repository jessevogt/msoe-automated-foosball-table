/*
 * RTLinux emulation
 *
 *  Copyright (C) 1999 FSM Labs (http://www.fsmlabs.com/)
 *  Written by Michael Barabanov <baraban@fsmlabs.com>
 */

#ifndef __RTL_RTL_H__
#define __RTL_RTL_H__
/* non-POSIX stuff for RTLinux kernel and emulation */

#ifdef MODULE
/* we're compiling for the native RTLinux */

#include <rtl_conf.h>
#include <linux/module.h>

int rtl_printf(const char * fmt, ...);
void conpr(const char *s);

#else
/* user-mode emulation of the RTLinux environment */

#include <stdio.h>
#include <stdarg.h>

static int rtl_printf(const char *fmt, ...)
{
	va_list list;
	int ret;
	va_start (list, fmt);
	ret = vprintf(fmt, list);
	va_end (list);
	return ret;
}

#endif

#endif
