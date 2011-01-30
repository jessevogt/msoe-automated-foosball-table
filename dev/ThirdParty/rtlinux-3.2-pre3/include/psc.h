/*
 * RTLinux user-level realtime internal support
 *
 * Copyright (C) 2000 FSM Labs (http://www.fsmlabs.com/)
 */
#ifndef _RTLINUX_PSC_H
#define _RTLINUX_PSC_H
extern int psc_active;

extern void rtl_make_psc_active(void);
extern void rtl_make_psc_inactive(void);
extern int  rtl_is_psc_active(void);

extern void psc_deliver_signal( int signal, struct task_struct *task );

#endif /* _RTLINUX_PSC_H */
