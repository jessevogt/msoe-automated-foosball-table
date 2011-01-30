/*
 * PPC-specific RTLinux switch support
 *
 * Copyright (C) 1999 Cort Dougan <cort@fsmlabs.com>
 */

#include <linux/threads.h>
#include <asm/system.h>

#include <rtl_conf.h>
#include <rtl_sched.h>
#include <rtl_core.h>

#ifdef CONFIG_RTL_FP_SUPPORT
void rtl_fpu_save (schedule_t *s, RTL_THREAD_STRUCT *current_t)
{
}

void rtl_fpu_restore (schedule_t *s,RTL_THREAD_STRUCT *current_t)
{
}

void rtl_task_init_fpu (RTL_THREAD_STRUCT *t, RTL_THREAD_STRUCT *fpu_owner)
{
}
#endif
