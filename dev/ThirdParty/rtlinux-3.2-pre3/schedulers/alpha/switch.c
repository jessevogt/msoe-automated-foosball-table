/*
 * Alpha-specific RTLinux switch support
 *
 * Copyright (C) 2000 Cort Dougan <cort@fsmlabs.com>
 */

#include <linux/threads.h>
#include <asm/system.h>

#include <rtl_conf.h>
#include <rtl_sched.h>
#include <rtl_core.h>

void rtl_fpu_save (schedule_t *s, RTL_THREAD_STRUCT *current_t)
{
}

void rtl_fpu_restore (schedule_t *s,RTL_THREAD_STRUCT *current_t)
{
}

void rtl_task_init_fpu (RTL_THREAD_STRUCT *t, RTL_THREAD_STRUCT *fpu_owner)
{
}

void rtl_switch_to(RTL_THREAD_STRUCT **current, RTL_THREAD_STRUCT *new)
{
	ulong __save(ulong);
	void __restore(ulong,ulong);	
	if ( __save((ulong)*current) )
		__restore((ulong)current,(ulong)new);
}
