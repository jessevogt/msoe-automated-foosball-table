/*
 * (C) Finite State Machine Labs Inc. 1999-2000 <business@fsmlabs.com>
 *
 * Released under the terms of GPL 2.
 * Open RTLinux makes use of a patented process described in
 * US Patent 5,995,745. Use of this process is governed
 * by the Open RTLinux Patent License which can be obtained from
 * www.fsmlabs.com/PATENT or by sending email to
 * licensequestions@fsmlabs.com
 */

#define RTL_NOWRAP

#include <rtl_conf.h>
#ifndef CONFIG_RTL_TRACER
#error Please enable CONFIG_RTL_TRACER option and copy new rtl.mk to this directory
#endif

#include <linux/kernel.h>
#include <linux/module.h>
#include <rtl_sync.h>
#include <rtl_time.h>
#include <rtl_core.h>
#include <rtl_tracer.h>
#include <mbuff.h>

static unsigned eventmask;
static int timestamp;

struct rtl_event_descriptor_struct {
	unsigned class;
} rtl_event_descriptor[RTL_TRACE_MAX_EVENTS];

int rtl_trace_seteventclass (int event, unsigned classmask)
{
	if ((unsigned) event >= RTL_TRACE_MAX_EVENTS) {
		return -1;
	}

	rtl_event_descriptor[event].class = classmask;
	return 0;
}

static struct rtl_trace_eventinfo_struct *rtl_trace_event_info = 0;

int rtl_trace_seteventname (int event, char *name)
{
	if ((unsigned) event >= RTL_TRACE_MAX_EVENTS) {
		return -1;
	}

	strncpy(rtl_trace_event_info[event].name, name, RTL_TRACER_NAMELEN - 1);
	rtl_trace_event_info[event].name[RTL_TRACER_NAMELEN - 1] = 0;
	return 0;
}

unsigned rtl_trace_settracemask (unsigned mask)
{
	unsigned oldmask = eventmask;
	eventmask = mask;
	rtl_printf("%x\n", eventmask);
	return oldmask;
}

static struct rtl_trace_buffer *buffers;
static struct rtl_trace_buffer *cbuf;
#define CBUF (*cbuf)
static spinlock_t buffspin;

static void (*save_rtl_trace)(int event_id, long event_data, void * eip);

static int len;
static struct rtl_trace_record *cur;
static struct rtl_trace_record *ebp;
static struct rtl_trace_record *sbp;

int rtl_trace_finalize (int id);

/* #include <asm/msr.h> */
static void rtl_do_trace(int event_id, long event_data, void *eip)
{
	rtl_irqstate_t flags;
	static int in_trace = 0;
	int cpu = rtl_getcpuid();

/*	if ((unsigned) event_id >= RTL_TRACE_MAX_EVENTS) {
		return;
	} */

	if (event_id == RTL_TRACE_FINALIZE) {
		rtl_trace_finalize(1);
		return;
	}
	rtl_no_interrupts(flags);
	if (test_and_set_bit (cpu, &in_trace)) {
		/* an attempt to call rtl_do_trace recursively */
		rtl_restore_interrupts (flags);
		return;
	}

	if (rtl_event_descriptor[event_id].class & eventmask) {
		spin_lock (&buffspin);

		cur->timestamp = gethrtime();
/* 		rdtscll(cur->timestamp); */
		cur->event_id = event_id;
		cur->eip = (long) eip;
		cur->event_data = event_data;
		cur->cpu = cpu;

		++cur;
		if (len < RTL_TNRECORDS) {
			len ++;
		}
		if (cur >= ebp) {
			cur = sbp;
		}
		
		spin_unlock (&buffspin);
	}
	clear_bit (cpu, &in_trace);
	rtl_restore_interrupts (flags);
}



static inline void init_current_buffer(void)
{
	struct rtl_trace_buffer *buff = &CBUF;
	buff->state = 0;
	len = 0;
	sbp = cur = &(buff->trace[0]);
	ebp = sbp + RTL_TNRECORDS;
}

/*
static void start_trace(void)
{
	if (rtl_trace_buffer != rtl_do_trace) {
	}
}

static void stop_trace(void)
{
	rtl_trace_buffer = save_rtl_trace;
}
*/

int rtl_trace_init (void *start)
{
	int i;

	for (i = 0; i < RTL_TRACE_MAX_EVENTS; i ++) {
		rtl_trace_seteventclass(i, RTL_TRACE_CLASS_DEFAULT);
	}
	for (i = RTL_TRACE_HARD_CLI; i < RTL_TRACE_SCHED_IN; i++) {
		rtl_trace_seteventclass(i, RTL_TRACE_CLASS_INTERRUPTS);
	}
	for (i = RTL_TRACE_SCHED_IN; i <= RTL_TRACE_SCHED_CTX_SWITCH; i++) {
		rtl_trace_seteventclass(i, RTL_TRACE_CLASS_SCHEDULER);
	}
	rtl_trace_seteventclass(RTL_TRACE_USER, RTL_TRACE_CLASS_USER);

	for (i = 0; i < RTL_TRACE_MAX_EVENTS; i ++) {
		rtl_trace_seteventname(i, "unknown");
	}

	rtl_trace_seteventname(RTL_TRACE_HARD_CLI, "hard cli");
	rtl_trace_seteventname(RTL_TRACE_HARD_STI, "hard sti");
	rtl_trace_seteventname(RTL_TRACE_HARD_SAVE_FLAGS, "rtl_hard_save_flags");
	rtl_trace_seteventname(RTL_TRACE_HARD_RESTORE_FLAGS, "rtl_restore_interrupts");
	rtl_trace_seteventname(RTL_TRACE_HARD_SAVEF_AND_CLI, "rtl_no_interrupts");
	rtl_trace_seteventname(RTL_TRACE_LOCAL_INTERCEPT, "local_intercept entry");
	rtl_trace_seteventname(RTL_TRACE_LOCAL_INTERCEPT_EXIT, "local_intercept exit");
	rtl_trace_seteventname(RTL_TRACE_USER, "user");
	rtl_trace_seteventname(RTL_TRACE_SPIN_LOCK, "rtl_spin_lock");
	rtl_trace_seteventname(RTL_TRACE_SPIN_UNLOCK, "rtl_spin_unlock");
	rtl_trace_seteventname(RTL_TRACE_INTERCEPT, "rtl_intercept entry");
	rtl_trace_seteventname(RTL_TRACE_INTERCEPT_EXIT, "rtl_intercept exit");
	rtl_trace_seteventname(RTL_TRACE_SCHED_IN, "scheduler in");
	rtl_trace_seteventname(RTL_TRACE_SCHED_OUT, "scheduler out");
	rtl_trace_seteventname(RTL_TRACE_SCHED_CTX_SWITCH, "rtl_switch_to");
	rtl_trace_seteventname(RTL_TRACE_FINALIZE, "trace finalize");

	eventmask = 0xffffffff;
	timestamp = 0;
	spin_lock_init (&buffspin);
	buffers = (struct rtl_trace_buffer *) start;
	for (i = 0; i < RTL_TNBUFFERS; i++) {
		buffers [i] . state = 0;
	}
	cbuf = &buffers[0];
	init_current_buffer ();
	save_rtl_trace = rtl_trace;
	rtl_trace = rtl_do_trace;
	return 0;
}

void rtl_trace_destroy (void)
{
	rtl_trace = save_rtl_trace;
}


/* fails if no more buffers available */
int rtl_trace_finalize (int id)
{
	struct rtl_trace_buffer *b;
	rtl_irqstate_t flags;

	rtl_no_interrupts (flags);
	spin_lock (&buffspin);

	++timestamp;
	for (b = buffers; b < &buffers[RTL_TNBUFFERS]; b ++) {
		if (b != cbuf && !b->state) {
			break;
		}
	}
	
	if (b == &buffers[RTL_TNBUFFERS]) {
		init_current_buffer (); /* start all over again */
		spin_unlock (&buffspin);
		rtl_restore_interrupts (flags);
		return -1;
	}

	CBUF.id = id;
	CBUF.timestamp = timestamp;
	CBUF.len = len;
	CBUF.pos = cur - sbp;
	mb();
	CBUF.state = 1;

	cbuf = b;
	init_current_buffer ();

	spin_unlock (&buffspin);
	rtl_restore_interrupts (flags);
	return 0;
}



#ifdef MODULE

/* struct rtl_trace_buffer traces[RTL_TNBUFFERS]; */

void * shmem_ptr;

int init_module(void)
{

	shmem_ptr = mbuff_alloc (RTL_TRACER_SHMEM, sizeof(struct rtl_trace_buffer) * RTL_TNBUFFERS);
	if (!shmem_ptr) {
		printk("RTLinux Tracer: failed to allocate trace buffer\n");
		return -1;
	}
	rtl_trace_event_info = (struct rtl_trace_eventinfo_struct *) mbuff_alloc (RTL_TRACER_SHMEM_INFO, sizeof(struct rtl_trace_eventinfo_struct) * RTL_TRACE_MAX_EVENTS);
	if (!rtl_trace_event_info) {
		printk("RTLinux Tracer: failed to allocate event info buffer\n");
		mbuff_free (RTL_TRACER_SHMEM, shmem_ptr);
		return -1;
	}
/* 	printk("%x %d\n", (unsigned) shmem_ptr, sizeof(struct rtl_trace_buffer)); */

	rtl_trace_init(shmem_ptr);
	return 0;
}

void cleanup_module(void)
{
	rtl_trace_destroy();
	mbuff_free (RTL_TRACER_SHMEM, shmem_ptr);
	mbuff_free (RTL_TRACER_SHMEM_INFO, (void *) rtl_trace_event_info);
}

#endif


