/*
 * RTLinux FIFO interface to GDB stub
 *
 * Written by Michael Barabanov (baraban@fsmlabs.com)
 * Copyright (C) 2000 Finite State Machine Labs Inc.
 * Released under the terms of the GPL Version 2.0
 *
 */

#include <rtl.h>
#include <rtl_conf.h>
#include <rtl_sync.h>
#include <rtl_core.h>
#include <rtl_fifo.h>
#include <rtl_sched.h>
#include <linux/ioctl.h>
#include <rtl_mutex.h>

/* #define RTL_DEBUG_PRINT */
#include <rtl_debug.h>

#define PSC_EXCEPTION 1
#ifdef PSC_EXCEPTION
#include <linux/module.h>
#endif

static int fifo = 10;

#define RTL_FIFO_FROM_GDB  (fifo)
#define RTL_FIFO_TO_GDB    (RTL_FIFO_FROM_GDB + 1)
#define RTL_FIFO_FROM_GDB2 (RTL_FIFO_FROM_GDB + 2)

static pthread_mutex_t gdb_lock = PTHREAD_MUTEX_INITIALIZER;
static int oldprio = 0;
static pthread_t oldprio_thread = 0;
int rtl_send_exception_info = 0;

/* find the module that supposedly contains a thread that caused an exception
 * and prepare offset information */
static unsigned long text;
static unsigned long data;
static unsigned long bss;
static struct module *last_module = 0;
static pthread_t last_thread = 0;
#if 0
/* support stuff for trapping in kernel system calls. -Nathan */
void (*old_sys_callp)(int);
extern void (*sys_call_table)(int);
extern int force_sig_info(int, struct siginfo *, struct task_struct *);
#endif

int rtl_search_module(struct module *mod)
{
	int i;
	text = 0;
	data = 0;
	bss = 0;

	if (!mod) {
		return -1;
	}
	for (i = 0; i < mod->nsyms; i++) {
		if (strncmp(mod->syms[i].name, "__insmod_", 9)) {
			continue;
		}
		if (strstr(mod->syms[i].name, "S.text")) {
			text = mod->syms[i].value;
		}
		if (strstr(mod->syms[i].name, "S.data")) {
			data = mod->syms[i].value;
		}
		if (strstr(mod->syms[i].name, "S.bss")) {
			bss = mod->syms[i].value;
		}
/* 		rtl_cprintf("%s %x\n", mod->syms[i].name, mod->syms[i].value); */

	}
	if (!text) {
		rtl_printf("rtl_debug warning: couldn't find section addresses; your insmod is too old\n");
		return -1;
	}
	if (!data) {
		data = text;
	}
	if (!bss) {
		bss = text;
	}

	return 0;
}

void rtl_exit_debugger(void *arg)
{
	int i;
	for (i = 0; i < rtl_num_cpus(); i++) {
		int cpu = cpu_logical_map (i);
		rtl_sched[cpu].rtl_linux_task.sched_param.sched_priority = -1;
#ifdef CONFIG_SMP
		if (cpu != rtl_getcpuid()) {
			rtl_reschedule(cpu);
		}
#endif
/* 		last_module = 0; */

	}
	oldprio_thread->sched_param.sched_priority = oldprio;
	pthread_mutex_unlock (&gdb_lock);
}


int rtl_enter_debugger(int exceptionVector, void *eip)
{
	struct module *mod;
	pthread_mutex_lock (&gdb_lock);
	oldprio = pthread_self()->sched_param.sched_priority;
	oldprio_thread = pthread_self();
	pthread_self()->sched_param.sched_priority = sched_get_priority_max(0) + 3;
	{
		int i;
		for (i = 0; i < rtl_num_cpus(); i++) {
			int cpu = cpu_logical_map (i);
			rtl_sched[cpu].rtl_linux_task.sched_param.sched_priority = sched_get_priority_max(0) + 1;
#ifdef CONFIG_SMP
			if (cpu != rtl_getcpuid()) {
				rtl_reschedule(cpu);
			}
#endif

		}

	}
	debugpr("passed\n");

	mod = pthread_self()->creator;

	if (mod && (last_module != mod)) {
		/* 		rtl_printf("rtl_debug: module=%#x lastmodule=%#x\n", mod, last_module); */
		if (&__this_module != pthread_self()->creator) {
			last_module = mod;
			last_thread = pthread_self();
			rtl_printf("rtl_debug: exception %#x in %s (EIP=%#x), thread id %#x; (re)start GDB to debug\n", exceptionVector, mod->name, eip, last_thread);
			rtl_search_module(mod);
		} else {
			rtl_printf("rtl_debug: interrupt (%#x)\n", exceptionVector);
		}
	}
	/* reply to host that an exception has occurred */
	if (rtf_isused(fifo)) {
		set_bit (0, &rtl_send_exception_info);
	}
	return 0;
}



#include "arch/rtl-stub.c"

static pthread_t waiting_thread = 0;
static spinlock_t fifo_lock = SPIN_LOCK_UNLOCKED;

int getDebugChar(void)
{
	char c;
	/* blocking FIFOs or mq_receive would be nice here */
	while (1) {
		rtl_irqstate_t flags;
		rtl_spin_lock_irqsave(&fifo_lock, flags);
		if (rtf_get(RTL_FIFO_FROM_GDB2, &c, 1) == 1) {
			rtl_spin_unlock_irqrestore(&fifo_lock, flags);
			break;
		}
/* 		debugpr("S"); */
		pthread_kill (pthread_self(), RTL_SIGNAL_SUSPEND);
		waiting_thread = pthread_self();
		rtl_spin_unlock_irqrestore(&fifo_lock, flags);

		rtl_schedule();
		pthread_testcancel();
	}

/* 	debugpr("%c", c); */
	return c;
}

int putDebugChar(int chr)
{
	char c = chr;
/* 	debugpr(">%c", c); */
	rtf_put (RTL_FIFO_TO_GDB, &c, 1);
	return 1;
}


pthread_t interrupt_threadid;

static int fifo_handler(unsigned int fifo)
{
	char c;

	if (rtf_get(RTL_FIFO_FROM_GDB, &c, 1) <= 0) {
		return 0;
	}
	if (c == 3)  {
		pthread_wakeup_np(interrupt_threadid);
	} else {
		rtl_irqstate_t flags;
		int cpu = -1;
		do {
			rtf_put(RTL_FIFO_FROM_GDB2, &c, 1);
		} while (rtf_get(RTL_FIFO_FROM_GDB, &c, 1) == 1);

		rtl_spin_lock_irqsave(&fifo_lock, flags);
		if (waiting_thread) {
			cpu = waiting_thread->cpu;
			pthread_kill (waiting_thread, RTL_SIGNAL_WAKEUP);
			waiting_thread = 0;
		}
		rtl_spin_unlock_irqrestore(&fifo_lock, flags);
		if (cpu >= 0) {
#ifdef CONFIG_SMP
			if (cpu != rtl_getcpuid())
				rtl_reschedule(cpu);
			else
#endif
				rtl_schedule();
		}
	}
	return 0;
}

static void *interrupt_thread(void *p)
{
	while (1) {
		pthread_suspend_np(pthread_self());
		debugpr("breakpoint\n");
		breakpoint();
	}
	return 0;
}

/* handle flushing */
static int gdb_fifo_ioctl(unsigned int fifo, unsigned int cmd, unsigned long arg)
{
	if (cmd == TCFLSH) {
		if (arg == TCIFLUSH) {
			rtf_flush (fifo + 1);
		}
	}

	return 0;
}
#if 0
/* handling of system calls in kernel for PSC programs. -Nathan */
void rtlinux_sys_call(int syscall_no) 
{
	if (is_psc_active()) {
		/* do something to send a signal back to the offending
		 * process 
		struct siginfo ignored;
		force_sig_info(SIGSEGV, &ignored, get_linux_current()); */
	} else /* just do whatever the syscall would do */
		old_sys_callp(syscall_no);
}
#endif

/* this function is used to set up exception handlers for tracing and
   breakpoints */
int set_debug_traps(void)
{
	if (rtl_debug_initialized) {
		printk("rtl_debug: already loaded\n");
		return -1;
	}

#ifdef PSC_EXCEPTION
	if(!inter_module_get("psc_exception_handler"))
		rtl_request_traps(&rtl_debug_exception);
	else /* decrement use-count */
		inter_module_put("psc_exception_handler");

	inter_module_register("rtl_debug_handler", THIS_MODULE, rtl_debug_exception);
#else
	rtl_request_traps(&rtl_debug_exception);
#endif

	rtl_debug_initialized = 1;
	return 0;

}

void unset_debug_traps(void)
{
	/*
	int i;
	for (i = 0; i < nbreak; i++) {
		debugpr ("unpatching leftover breakpoints\n");
		set_char(bp_cache[i].mem, bp_cache[i].val);
	}
	*/

#ifdef PSC_EXCEPTION
	inter_module_unregister("rtl_debug_handler");
	if(!inter_module_get("psc_exception_handler"))
		rtl_request_traps(0);
	else /* decrement use-count */
		inter_module_put("psc_exception_handler");

#else
	rtl_request_traps(0);
#endif

	rtl_debug_initialized = 0;
}

#ifdef MODULE

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Michael Barabanov <baraban@fsmlabs.com>");
MODULE_DESCRIPTION("RTLinux Debugger");
MODULE_PARM(fifo, "i");
static int bp = 0;
MODULE_PARM(bp, "i");
static int quiet = 0;
MODULE_PARM(quiet, "i");

#define CREATE_FIFO(f, sz) \
do { \
	int ret = rtf_create((f), (sz)); \
	if (ret < 0) { \
		printk("rtl_debug: failed to create FIFO %d (%d)\n", (f), ret); \
		return -1; \
	} \
} while (0)

int init_module(void)
{
	pthread_attr_t attr;
	struct sched_param sched_param;

	pthread_attr_init (&attr);
	sched_param.sched_priority = sched_get_priority_max(0) + 2;
	pthread_attr_setschedparam (&attr, &sched_param);

	CREATE_FIFO (RTL_FIFO_FROM_GDB, 4000);
	CREATE_FIFO (RTL_FIFO_TO_GDB, 4000);
	CREATE_FIFO (RTL_FIFO_FROM_GDB2, 4000);
	rtf_make_user_pair (RTL_FIFO_FROM_GDB, RTL_FIFO_TO_GDB);
	rtf_link_user_ioctl (RTL_FIFO_FROM_GDB, gdb_fifo_ioctl);

	rtf_create_handler(RTL_FIFO_FROM_GDB, fifo_handler);
	pthread_create (&interrupt_threadid, &attr, interrupt_thread, NULL);

	if (set_debug_traps()) {
		pthread_cancel (interrupt_threadid);
		pthread_join (interrupt_threadid, NULL);
		rtf_destroy(RTL_FIFO_FROM_GDB);
		rtf_destroy(RTL_FIFO_TO_GDB);
		rtf_destroy(RTL_FIFO_FROM_GDB2);
		return -1;
	}
	if ( !quiet ) {
		printk("RTLinux Debugger Loaded (http://www.fsmlabs.com/)\n");
	}
	if (bp) {
		pthread_wakeup_np(interrupt_threadid);
	}

	return 0;
#if 0
	/* Support for catching system calls in kernel. -Nathan */
#ifdef CONFIG_SMP
#warning "Trapping system calls in kernel isn't SMP safe yet."
#warning "Just try not to make any system calls from kernel space :-)"
#endif /* CONFIG_SMP */
	old_sys_callp = sys_call_table;
	sys_call_table = &rtlinux_sys_call;
#endif
}

void cleanup_module(void)
{
	pthread_delete_np (interrupt_threadid);
	rtf_destroy(RTL_FIFO_FROM_GDB);
	rtf_destroy(RTL_FIFO_TO_GDB);
	rtf_destroy(RTL_FIFO_FROM_GDB2);
	unset_debug_traps();
#if 0
	/* Unhook system calls. -Nathan */
	sys_call_table = old_sys_callp;
#endif
}

#endif
