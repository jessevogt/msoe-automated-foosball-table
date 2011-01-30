/*
 * (C) Finite State Machine Labs Inc. 2000 business@fsmlabs.com
 *
 * Released under the terms of GPL 2.
 * Open RTLinux makes use of a patented process described in
 * US Patent 5,995,745. Use of this process is governed
 * by the Open RTLinux Patent License which can be obtained from
 * www.fsmlabs.com/PATENT or by sending email to
 * licensequestions@fsmlabs.com
 *
 * July 2003, Nils Hasler <nils@penai.de>
 *    Added exception-handling
 */

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/signal.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/mmu_context.h>

#include <rtl_debug.h>
#include <rtl_core.h>
#include <rtlinux_signal.h>
#include <psc.h>
#include <rtl_sync.h>
#include <rtl_time.h>
#include <rtl_sched.h>
#include <rtl_fifo.h>
#include <arch/rtl_switch.h>

#define PSC_EXCEPTION 1

#ifdef PSC_EXCEPTION
struct task_struct *psc_current;
#endif

/* posix/signal.h defines these.  It's an ugly hack and this
 * is a ugly workaround. -- Cort
 */
#undef sa_sigaction
#undef sa_handler
#undef sigaction

MODULE_AUTHOR("FSMLabs <support@fsmlabs.com>");
MODULE_DESCRIPTION("RTLinux user-level real-time Module");
MODULE_LICENSE("GPL v2");

#define PSC_DEVNAME "RTLinux user-level IRQ handler"

#define rtl_get_debugger()	inter_module_get("rtl_debug_handler")
#define rtl_release_debugger()  inter_module_put("rtl_debug_handler")

extern long sys_call_table[256];
unsigned long sys_ni_syscall;

unsigned long __NR_rtf_destroy = 0;
unsigned long __NR_rtf_create = 0;

static int psc_count = 0;

struct proc_dir_entry *proc_rtlinux, *proc_sigaction,
    *proc_sigprocmask, *proc_gethrtime, *proc_rtf_put,
    *proc_rtf_get, *proc_rtf_create, *proc_rtf_destroy;

struct irq_thread {
	struct rtlinux_sigaction action;
	pid_t pid;
	pthread_t ptid;
} irq_threads[NR_IRQS];

struct timer_thread {
	struct rtlinux_sigaction action;
	pid_t pid;
	pthread_t ptid;
	struct timer_thread *next;
	int pending;
} *timer_threads;

/* support function for checking signal masks. -Nathan */
int rtlinux_sigismember(const rtlinux_sigset_t * set, int sig)
{
	int offset;
	unsigned long int value;

	if (set == NULL)
		return (-1);
	if (sig > RTLINUX_SIGMAX) {
		errno = EINVAL;
		return (-1);
	}

	offset = (int) ((sig / (sizeof(unsigned long int) * 8)));
	value = 1 << (sig % (sizeof(unsigned long int) * 8));

	if (((unsigned long int) (set->__val[offset] & value)) > 0)
		return (1);
	return (0);
}

int rtlinux_sigemptyset(rtlinux_sigset_t * set)
{
	if (set == NULL)
		return (-1);
	memset(set, 0x00, sizeof(rtlinux_sigset_t));
	return (0);
}

static void insert_timer_thread(struct timer_thread *thread)
{
	unsigned long flags;

	rtl_hard_savef_and_cli(flags);

	rtlinux_sigemptyset(&(thread->action.sa_mask));
	thread->pending = 0;

	thread->next = timer_threads;
	timer_threads = thread;

	rtl_hard_restore_flags(flags);
}

static struct timer_thread *remove_timer_thread(struct timer_thread *tt)
{
	unsigned long flags;
	struct timer_thread *ret = NULL;

	rtl_hard_savef_and_cli(flags);

	if (tt == timer_threads) {
		timer_threads = timer_threads->next;
		goto out;
	}

	for (ret = timer_threads; ret->next && (ret->next != tt);
	     ret = ret->next)
		/* nothing */ ;
	if (ret->next)
		ret->next = ret->next->next;
      out:
	kfree(tt);
	rtl_hard_restore_flags(flags);
	return ret;
}

static struct timer_thread *find_timer_thread(pid_t pid, int signal)
{
	unsigned long flags;
	struct timer_thread *ret;

	rtl_hard_savef_and_cli(flags);

	for (ret = timer_threads; ret && ((ret->pid != pid) ||
	     (ret->action.sa_signal != signal)); ret = ret->next)
		/* nothing */ ;

	rtl_hard_restore_flags(flags);
	return ret;
}

static int call_handler(struct rtlinux_sigaction *act, pid_t pid)
{
	rtl_irqstate_t flags;
#ifdef RTL_PSC_NEW
	rtl_mmu_state_t mmu_state;
#else
	struct task_struct *linux_current = get_linux_current();
#endif
	struct task_struct *tsk;
	int ret = 0;

	/* make sure the task that wants this interrupt is still running */
	if (!(tsk = find_task_by_pid(pid))) {	/* TODO is this really safe? */
		/* make sure the handler is disabled */
		act->sa_flags |= RTLINUX_SA_ONESHOT;
		rtl_printf("task %d is gone!\n", pid);
		ret = -1;
		goto out;
	}

	if (!tsk->mm) {
		/* make sure the handler is disabled */
		act->sa_flags |= RTLINUX_SA_ONESHOT;
		rtl_printf("task has no ->mm, not calling\n");
		ret = -1;
		goto out;
	}
#ifndef RTL_PSC_NEW
	if (!linux_current->active_mm) {
		act->sa_flags |= RTLINUX_SA_ONESHOT;
		rtl_printf("current has no ->mm, not calling\n");
		ret = -1;
		goto out;
	}
#endif

	rtl_no_interrupts(flags);

	if(!psc_count)
		rtl_make_psc_active();
	psc_count++;

#ifdef PSC_EXCEPTION
	psc_current = tsk;
#endif

#ifdef RTL_PSC_NEW

	rtl_mmu_save(&mmu_state);
	rtl_mmu_switch_to(tsk);

	if((act->sa_flags & RTLINUX_SA_MULTIPSC))
		rtl_restore_interrupts(flags);

	act->sa_handler(act->sa_signal); /* actually do call the */

	if((act->sa_flags & RTLINUX_SA_MULTIPSC))
		rtl_no_interrupts(flags);

	rtl_mmu_restore(&mmu_state);

#else
	switch_mm(linux_current->active_mm, tsk->mm, tsk, rtl_getcpuid());

	if((act->sa_flags & RTLINUX_SA_MULTIPSC))
		rtl_restore_interrupts(flags);

	act->sa_handler(act->sa_signal); /* actually do call the */

	if((act->sa_flags & RTLINUX_SA_MULTIPSC))
		rtl_no_interrupts(flags);

	switch_mm(tsk->mm, linux_current->active_mm, tsk, rtl_getcpuid());
#endif

#ifdef PSC_EXCEPTION
	psc_current = 0;
#endif

	psc_count--; /* save because interrupts are disabled */
	if(!psc_count)
		rtl_make_psc_inactive();

	rtl_restore_interrupts(flags);
out:
	return ret;
}

static void *psc_irq_thread(void *t)
{
	ulong flags;
	struct irq_thread *my_irq_thread = (struct irq_thread *) t;

	rtl_stop_interrupts();
	while (1) {
		pthread_suspend_np(pthread_self());

		/* if the handler call fails or we don't want to reset the
		 * handler remove it */
		if (call_handler
		    (&my_irq_thread->action, my_irq_thread->pid)
		    || (my_irq_thread->
			action.sa_flags & RTLINUX_SA_ONESHOT)) {
			/* we are synchronizing with RTL, so hard cli */
			rtl_hard_savef_and_cli(flags);

			rtl_free_global_irq(my_irq_thread->
					    action.sa_signal);
			my_irq_thread->pid = 0;

			rtl_hard_restore_flags(flags);
		}
		rtl_global_pend_irq(my_irq_thread->action.sa_signal);
		rtl_hard_enable_irq(my_irq_thread->action.sa_signal);
	}
}

static void *timer_handler(void *t)
{
	struct timer_thread *ptr = (struct timer_thread *) t;
	struct sched_param p;
	p.sched_priority = ptr->action.sa_priority;
	pthread_setschedparam(pthread_self(), SCHED_FIFO, &p);

	if (pthread_make_periodic_np(pthread_self(),
				     gethrtime() + ptr->action.sa_period,
				     ptr->action.sa_period))
		    rtl_printf("Error from pthread_make_periodic_np()\n");
	while (1) {
		pthread_wait_np();
		/* if the handler call fails or we don't want to reset the handler
		 * remove it
		 */
		if (ptr->pending == 0) {
			if (call_handler(&ptr->action, ptr->pid) ||
			    (ptr->action.sa_flags & RTLINUX_SA_ONESHOT)) {
				remove_timer_thread(ptr);
				pthread_exit(NULL);
			}
		} else
			ptr->pending++;
	}

	return NULL;
}

static unsigned int psc_irq_handler(unsigned int irq, struct pt_regs *regs)
{
	/* just wake the stinking IRQ thread up and let it handle it -Nathan */
	pthread_wakeup_np(irq_threads[irq].ptid);
	return 0;
}

static int sigaction_write(struct file *file, const char *buffer,
			   unsigned long count, void *data)
{
	struct rtlinux_sigaction action;
	ulong flags;
	struct timer_thread *tt = NULL;
	int retval;

	if (count != sizeof(action))
		return -EINVAL;

	/* make sure the buffer we were passed is good, then copy it */
	if (copy_from_user
	    ((void *) &action, (void *) buffer,
	     sizeof(action))) return -EFAULT;

	rtlinux_sigemptyset(&(action.sa_mask));

	/* if the signal is < SIGTIMER0 it's an IRQ request */
	if (action.sa_signal < RTLINUX_SIGTIMER0) {
		/* a request to free the irq */
		if ((action.sa_handler == RTLINUX_SIG_DFL) ||
		    (action.sa_handler == RTLINUX_SIG_IGN)) {
			pthread_kill(irq_threads[action.sa_signal].ptid,
				     RTL_SIGNAL_CANCEL);
			if (rtl_free_global_irq(action.sa_signal))
				return -EALREADY;
			else
				return count;
		}

		/* we are synchronizing with RTL, so hard cli */
		rtl_hard_savef_and_cli(flags);

		/* grab the interrupt */
		if (rtl_request_global_irq
		    (action.sa_signal, psc_irq_handler)) {
			rtl_hard_restore_flags(flags);
			return -EBUSY;
		}

		/* hmm, somehow we need to threadify IRQs so they have state
		 * so they can be switched and stuff.  hmmm . . .  -Nathan */
		/* we now own the real-time irq, assign the code */
		irq_threads[action.sa_signal].pid = current->pid;
		irq_threads[action.sa_signal].action = action;
		if((retval=pthread_create(&irq_threads[action.sa_signal].ptid, NULL,
			       psc_irq_thread,
			       (void *) &irq_threads[action.sa_signal])) != 0) {
			rtl_free_global_irq(action.sa_signal);
			rtl_hard_restore_flags(flags);
			return -retval;
		}
		pthread_detach(irq_threads[action.sa_signal].ptid);
#ifdef CONFIG_RTL_FP_SUPPORT
		pthread_setfp_np(irq_threads[action.sa_signal].ptid, 1);
#endif				/* CONFIG_RTL_FP_SUPPORT */

		rtl_hard_restore_flags(flags);
	}
	/* is it a timer? */
	else if (action.sa_signal < RTLINUX_SIGUSR0) {
		pthread_attr_t attr;
		struct sched_param sp;
		/* if we're doing a DFL/IGN we need to kill the task
		 * and if we're changing the handler we need to kill it
		 */
		if (
		    (tt =
		     find_timer_thread(current->pid, action.sa_signal))) {
			pthread_kill(tt->ptid, RTL_SIGNAL_CANCEL);
			/* This is bad when this thread does bad things.
			 * -Nathan
			 * pthread_join( tt->ptid, NULL );
			 * Now threads are detached on creation.
			 * So, no need for pthread_join()
			 * -nils */
			remove_timer_thread(tt);
		}
		/* check if we're free-ing this timer signal */
		if ((action.sa_handler == RTLINUX_SIG_DFL) ||
		    (action.sa_handler == RTLINUX_SIG_IGN))
			goto out;

		if (!
		    (tt =
		     kmalloc(sizeof(struct timer_thread),
			     GFP_KERNEL))) return -ENOMEM;

		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		if((action.sa_flags & RTLINUX_SA_MULTIPSC)) {
			/* sanity check */
			if(action.sa_priority < 0 ||
			   action.sa_priority > sched_get_priority_max(SCHED_FIFO)) {
				kfree(tt);
				return -EFAULT;
			}
		}
		else /* default priority */
			action.sa_priority = 1;

		tt->pid = current->pid;
		tt->action = action;
		tt->pending = 0;

		sp.sched_priority = action.sa_priority;
		pthread_attr_setschedparam(&attr, &sp);
		if((retval=pthread_create(&tt->ptid, &attr, timer_handler,
					  (void *)tt)) != 0) {
			kfree(tt);
			return -retval;
		}
#ifdef CONFIG_RTL_FP_SUPPORT
		pthread_setfp_np(tt->ptid, 1);
#endif				/* CONFIG_RTL_FP_SUPPORT */
		insert_timer_thread(tt);
	}
	/* is it a signal for general use? */
	else if (action.sa_signal <= RTLINUX_SIGUSR3) {
		/*
		 * We do things here a lot like we do with timers so we can
		 * use the same housekeeping functions.
		 *   -- Cort
		 */

		/* if we're doing a DFL/IGN we need to remove the timer
		 * and if we're changing the handler we need to kill it
		 */
		if (
		    (tt =
		     find_timer_thread(current->pid,
				       action.sa_signal)))
		       remove_timer_thread(tt);

		/* check if we're free-ing this timer signal */
		if ((action.sa_handler == RTLINUX_SIG_DFL) ||
		    (action.sa_handler == RTLINUX_SIG_IGN))
			goto out;

		if (!
		    (tt =
		     kmalloc(sizeof(struct timer_thread),
			     GFP_KERNEL))) return -ENOMEM;
		tt->pid = current->pid;
		tt->action = action;
#ifdef CONFIG_RTL_FP_SUPPORT
/* 		pthread_setfp_np(tt->ptid, 1); *//* TODO */
#endif				/* CONFIG_RTL_FP_SUPPORT */
		insert_timer_thread(tt);
	} else {
		return -EINVAL;
	}

      out:

	return count;
}

/* This is roughly modeled after the sigaction_write function above. 
 * -Nathan */
static int sigprocmask_write(struct file *file, const char *buffer,
			     unsigned long count, void *data)
{
	unsigned long flags;
	rtlinux_sigset_t our_procmask;
	struct timer_thread *our_thread;
	int i;

	/* make sure we got everything we should have */
	if (count != sizeof(our_procmask))
		return (-EINVAL);

	/* get the bitmask from the syscall */
	if (copy_from_user((void *) &our_procmask, (void *) buffer,
			   sizeof(rtlinux_sigset_t))) return (-EFAULT);

	/* should we disable interrupts here?  Yeah, probably. */
	rtl_hard_savef_and_cli(flags);

	/* go through all the irq threads and find the ones specific to this
	 * pid and set their signal action masks */
	for (i = 0; i < NR_IRQS; i++) {
		if (irq_threads[i].pid == current->pid)
			memcpy(&(irq_threads[i].action.sa_mask),
			       &our_procmask, sizeof(rtlinux_sigset_t));
	}

	/* go through all the timer threads and find the ones specific to
	 * this pid and set their signal action masks */
	for (our_thread = timer_threads; our_thread;
	     our_thread = our_thread->next) {
		if (our_thread->pid == current->pid) {
			memcpy(&(our_thread->action.sa_mask),
			       &our_procmask, sizeof(rtlinux_sigset_t));
			our_thread->pending = 0;
			/* for each of the timer signals */
			for (i = NR_IRQS; i < RTLINUX_SIGMAX; i++) {
				/* if this signal is our mask, pend it */
				our_thread->pending |=
				    rtlinux_sigismember(&our_procmask, i);
			}
		}
	}

	/* . . . and re-enable interrupts here. -Nathan */
	rtl_hard_restore_flags(flags);

	/* go through all the interrupts and make sure we disable the ones we
	 * don't want */
	for (i = 0; i < NR_IRQS; i++) {
		if ((rtlinux_sigismember(&our_procmask, i)) == 1) {
			rtl_hard_disable_irq(i);
		}
		/* whilst enabling the ones we do want */
		else
			rtl_hard_enable_irq(i);
	}

	return (count);
}

int gethrtime_read(char *page, char **start, off_t off, int count,
		   int *eof, void *data)
{
	int size = 0;
	size = sprintf(page, "%lu", (unsigned long) gethrtime);
	return (size);
}

int rtf_put_read(char *page, char **start, off_t off, int count, int *eof,
		 void *data)
{
	int size = 0;
	size = sprintf(page, "%lu", (unsigned long) rtf_put);
	return (size);
}

int rtf_get_read(char *page, char **start, off_t off, int count, int *eof,
		 void *data)
{
	int size = 0;
	size = sprintf(page, "%lu", (unsigned long) rtf_get);
	return (size);
}

int rtf_destroy_read(char *page, char **start, off_t off, int count,
		     int *eof, void *data)
{
	int size = 0;
	size = sprintf(page, "%lu", __NR_rtf_destroy);
	return (size);
}

int rtf_create_read(char *page, char **start, off_t off, int count,
		    int *eof, void *data)
{
	int size = 0;
	size = sprintf(page, "%lu", __NR_rtf_create);
	return (size);
}

/* grab a free syscall entry and own it. -Nathan */

typedef int (*rtlinux_syscall_t) (int, ...);

int hook_free_syscall(rtlinux_syscall_t rtlinux_syscall)
{
	int __NR_rtlinux = 254;
	sys_ni_syscall = sys_call_table[255];

	while ((sys_call_table[__NR_rtlinux] != (unsigned
						 long) sys_ni_syscall))
		    __NR_rtlinux--;
	sys_call_table[__NR_rtlinux] = (unsigned long) rtlinux_syscall;

	return __NR_rtlinux;
}

int rtf_create_wrapper(unsigned int fifo, int size)
{
	rtl_irqstate_t flags;
	int ret = 0;
	rtl_hard_savef_and_cli(flags);
	ret = rtf_create(fifo, size);
	rtl_hard_restore_flags(flags);
	return ret;
}

int rtf_destroy_wrapper(unsigned int minor)
{
	rtl_irqstate_t flags;
	int ret = 0;
	rtl_hard_savef_and_cli(flags);
	ret = rtf_destroy(minor);
	rtl_hard_restore_flags(flags);
	return ret;
}

#ifdef PSC_EXCEPTION
#include "arch/psc-arch.c"

void exit_thread_now(void)
{
	/* rtl_cprintf("committing suicide\n"); */
	pthread_exit(0);
}

int psc_exception_handler(int vector, struct pt_regs *regs, int error_code)
{
	int signo;
	int (*debugger)(int, struct pt_regs *, int);

	debugpr("v %d ec %x\n", vector, error_code);

	signo = compute_signal();

	/* first the debugger gets a chance */
	if((debugger = rtl_get_debugger())) {
		if(debugger(vector, regs, error_code) || signo == SIGTRAP) {
			rtl_release_debugger(); /* decrement usecount */
			return 1;
		}
		rtl_release_debugger(); /* decrement usecount */
	}

	/* we are only responsible for psc */
	if(!rtl_is_psc_active())
		return 0;

	/* kill the faulting thread... */
	regs->eip = (long) exit_thread_now;

	/* ...and most probably coredump - but that is not our problem */
	if(psc_current)
		force_sig(signo, psc_current);
	else
		rtl_printf("ERROR psc_current == 0, NOT sending signal %d\n", signo);

	psc_current = 0;
	rtl_make_psc_inactive();

	return 1;
}
#endif /* PSC_EXCEPTION */

int init_module(void)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,0)
	proc_rtlinux = proc_mkdir("rtlinux", 0);
#else
	proc_rtlinux = create_proc_entry("rtlinux", S_IFDIR, 0);
#endif

	proc_sigaction = create_proc_entry("sigaction", S_IFREG | S_IWUSR,
					   proc_rtlinux);
	proc_sigaction->write_proc = sigaction_write;

	proc_sigprocmask =
	    create_proc_entry("sigprocmask", S_IFREG | S_IWUSR,
			      proc_rtlinux);
	proc_sigprocmask->write_proc = sigprocmask_write;

	proc_gethrtime = create_proc_entry("gethrtime", S_IFREG | S_IRUSR,
					   proc_rtlinux);
	proc_gethrtime->read_proc = gethrtime_read;

	proc_rtf_put = create_proc_entry("rtf_put", S_IFREG | S_IRUSR,
					 proc_rtlinux);
	proc_rtf_put->read_proc = rtf_put_read;

	proc_rtf_get = create_proc_entry("rtf_get", S_IFREG | S_IRUSR,
					 proc_rtlinux);
	proc_rtf_get->read_proc = rtf_get_read;

	/* setup rtf_create syscall -Nathan */
	__NR_rtf_create =
	    hook_free_syscall((rtlinux_syscall_t) rtf_create_wrapper);
	proc_rtf_create =
	    create_proc_entry("rtf_create", S_IFREG | S_IRUSR,
			      proc_rtlinux);
	proc_rtf_create->read_proc = rtf_create_read;

	/* setup rtf_destroy syscall -Nathan */
	__NR_rtf_destroy =
	    hook_free_syscall((rtlinux_syscall_t) rtf_destroy_wrapper);
	proc_rtf_destroy =
	    create_proc_entry("rtf_destroy", S_IFREG | S_IRUSR,
			      proc_rtlinux);
	proc_rtf_destroy->read_proc = rtf_destroy_read;

#ifdef PSC_EXCEPTION
	rtl_request_traps(psc_exception_handler);
	inter_module_register("psc_exception_handler", THIS_MODULE, psc_exception_handler);
#endif

	return 0;
}

void cleanup_module(void)
{
	int i;
#ifdef PSC_EXCEPTION
	int (*debugger)(int, struct pt_regs *, int);
#endif

	remove_proc_entry("sigprocmask", proc_rtlinux);
	remove_proc_entry("sigaction", proc_rtlinux);
	remove_proc_entry("gethrtime", proc_rtlinux);
	remove_proc_entry("rtf_put", proc_rtlinux);
	remove_proc_entry("rtf_get", proc_rtlinux);
	remove_proc_entry("rtf_create", proc_rtlinux);
	remove_proc_entry("rtf_destroy", proc_rtlinux);

	/* unsetup rtf_create syscall */
	remove_proc_entry("rtf_create", proc_rtlinux);
	sys_call_table[__NR_rtf_create] = sys_ni_syscall;

	/* unsetup rtf_destroy syscall */
	remove_proc_entry("rtf_destroy", proc_rtlinux);
	sys_call_table[__NR_rtf_destroy] = sys_ni_syscall;

	remove_proc_entry("rtlinux", &proc_root);

	/* free all the irq's we've taken */
	for (i = 0; i < NR_IRQS; i++) {
		if (irq_threads[i].pid) {
			rtl_free_irq(i);
			force_sig(SIGTERM,
				  find_task_by_pid(irq_threads[i].pid));
		}
	}

	/* free all the timers we've taken and kill the task */
	while (timer_threads) {
		pthread_kill(timer_threads->ptid, RTL_SIGNAL_CANCEL);
		/* This is bad when this thread does bad things.
		 * -Nathan
		 * pthread_join( timer_threads->ptid, NULL ); */
		remove_timer_thread(timer_threads);
	}

#ifdef PSC_EXCEPTION
	/* if debugger is started pass exceptions directly */
	debugger = rtl_get_debugger();
	if(debugger)
		rtl_release_debugger();
	rtl_request_traps(debugger);
	inter_module_unregister("psc_exception_handler");
#endif
}

void psc_deliver_signal(int signal, struct task_struct *tsk)
{
	struct timer_thread *tt = find_timer_thread(tsk->pid, signal);

	if (!tt)
		return;

	/* call the handler and remove it if 1) fails or 2) is oneshot */
	if (call_handler(&tt->action, tsk->pid) ||
	    (tt->action.sa_flags & RTLINUX_SA_ONESHOT))
		    remove_timer_thread(tt);
}
