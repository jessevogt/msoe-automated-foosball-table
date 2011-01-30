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

#include <stdio.h>
#include <signal.h>
#include <rtl_tracer.h>
#include "tracedump.h"


#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <mbuff.h>

struct rtl_trace_buffer *rtl_buffer;
struct rtl_trace_eventinfo_struct *rtl_trace_event_info = 0;

void do_rtl_dump(const char *stuff)
{
	printf(stuff);
}

static void cleanup(int signo)
{
	fprintf(stderr, "tracer cleanup (signal %d)\n", signo);
	mbuff_detach (RTL_TRACER_SHMEM, rtl_buffer);
	mbuff_detach (RTL_TRACER_SHMEM_INFO, rtl_trace_event_info);
	exit(0);
}


int main()
{
	int i;

	if (signal(SIGSEGV, SIG_IGN) != SIG_IGN) {
		signal(SIGSEGV, cleanup);
	}
	if (signal(SIGINT, SIG_IGN) != SIG_IGN) {
		signal(SIGINT, cleanup);
	}
	if (signal(SIGTERM, SIG_IGN) != SIG_IGN) {
		signal(SIGTERM, cleanup);
	}
	if (signal(SIGHUP, SIG_IGN) != SIG_IGN) {
		signal(SIGHUP, cleanup);
	}
	rtl_buffer = (struct rtl_trace_buffer *) mbuff_attach (RTL_TRACER_SHMEM, sizeof(struct rtl_trace_buffer) * RTL_TNBUFFERS);
	if (!rtl_buffer) {
		fprintf(stderr, "accessing mbuff failed\n");
		return -1;
	}

	rtl_trace_event_info = (struct rtl_trace_eventinfo_struct *) mbuff_attach (RTL_TRACER_SHMEM_INFO, sizeof(struct rtl_trace_eventinfo_struct) * RTL_TRACE_MAX_EVENTS);
	if (!rtl_trace_event_info) {
		fprintf(stderr, "accessing mbuff failed\n");
		mbuff_detach (RTL_TRACER_SHMEM, rtl_buffer);
		return -1;
	}

	while (1) {
		for (i = 0; i < RTL_TNBUFFERS; i ++)
		{
			if (rtl_buffer[i].state) {
				rtl_trace_dump(&rtl_buffer[i], &do_rtl_dump);
				rtl_buffer[i] . state = 0;
				fflush(stdout);
			}
		}
		sleep(1);
	}

	return 0;
}


