/*
 * CLI tracer
 * Written by Michael Barabanov (baraban@fsmlabs.com)
 * Copyright (C) Finite State Machine Labs Inc., 1999,2000
 * Released under the terms of the GNU GPL Version 2.
 */

#ifndef __RTL_TRACE_INT_H__
#define __RTL_TRACE_INT_H__

#define RTL_TRACE_MAX_EVENTS 64
#define RTL_TNBUFFERS 20
#define RTL_TNRECORDS 500

#define RTL_TRACER_FIFO 14

enum rtl_tracer_events_enum {
      	RTL_TRACE_HARD_CLI, RTL_TRACE_HARD_STI, RTL_TRACE_HARD_SAVE_FLAGS,
       	RTL_TRACE_HARD_RESTORE_FLAGS, RTL_TRACE_HARD_SAVEF_AND_CLI,

	RTL_TRACE_INTERCEPT, RTL_TRACE_INTERCEPT_EXIT,
       	RTL_TRACE_LOCAL_INTERCEPT, RTL_TRACE_LOCAL_INTERCEPT_EXIT,
	RTL_TRACE_SPIN_LOCK, RTL_TRACE_SPIN_UNLOCK, 

	RTL_TRACE_SCHED_IN, RTL_TRACE_SCHED_OUT, RTL_TRACE_SCHED_CTX_SWITCH,

	RTL_TRACE_FINALIZE,
	RTL_TRACE_USER };

#define RTL_TRACE_CLASS_DEFAULT 1
#define RTL_TRACE_CLASS_INTERRUPTS 2
#define RTL_TRACE_CLASS_USER 4
#define RTL_TRACE_CLASS_SCHEDULER 8


#define RTL_TRACER_NAMELEN 30
struct rtl_trace_eventinfo_struct {
	char name[RTL_TRACER_NAMELEN];
};

#ifdef CONFIG_RTL_TRACER

extern int rtl_trace_seteventclass (int event, unsigned classmask);
extern int rtl_trace_seteventname (int event, char *name);

extern void (*rtl_trace)(int event_id, long event_data, void *eip);
extern void rtl_trace2(int event_id, long event_data);

#ifndef RTL_NOWRAP
#include <rtl_tracewrap.h>
#endif

#else
#define rtl_trace(event_id, event_data, eip) do { ; } while (0)
#define rtl_trace2(event_id, event_data) do { ; } while (0)
#endif

#define RTL_TRACER_SHMEM "tracer"
#define RTL_TRACER_SHMEM_INFO "tracerinfo"

struct rtl_trace_record {
	long long timestamp;
	int event_id;
	long event_data;
	long event_data2;
	long event_data3;
	int cpu;
	long eip;
};


struct rtl_trace_buffer {
	int state;
	int pos; /* index of the current writing position */
	int timestamp;
	int len;
	int id;
	struct rtl_trace_record trace[RTL_TNRECORDS];
};


/* #ifdef __KERNEL__ */

extern int rtl_trace_init (void *start); /* and the tracing begins */
extern unsigned rtl_trace_settracemask (unsigned mask);
extern void rtl_trace_destroy (void);

/* #endif */

extern void rtl_trace_dump (struct rtl_trace_buffer *buffer, void (*dump)(const char *string));

#endif
