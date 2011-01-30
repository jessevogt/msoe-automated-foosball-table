/*
 * RTLinux core features.
 *
 * Copyright (C) 1999 FSM Labs (http://www.fsmlabs.com/)
 *  Written by Cort Dougan <cort@fsmlabs.com>
 */

#ifndef _ARCH_RTL_SWITCH_H_
#define _ARCH_RTL_SWITCH_H_

extern void rtl_fpu_save (schedule_t *s, RTL_THREAD_STRUCT *task);
extern void rtl_fpu_restore (schedule_t *s,RTL_THREAD_STRUCT *task);
void rtl_switch_to(RTL_THREAD_STRUCT **, RTL_THREAD_STRUCT *);
void __rtl_init_state(ulong, ulong, ulong, ulong);

#define rtl_init_stack(task,fn,data,rt_startup) \
	__rtl_init_state((ulong)task,(ulong)fn,(ulong)data,(ulong)rt_startup)

#endif /* _ARCH_RTL_SWITCH_H_ */
