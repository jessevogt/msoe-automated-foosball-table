/*
 * RTLinux irq test
 *
 * measures semaphore wakeup operation performance
 *
 *  (C) FSMLabs  2000. Michael Barabanov <baraban@fsmlabs.com>
 *  Released under the GNU GENERAL PUBLIC LICENSE Version 2, June 1991
 *  Any use of this code must include this notice.
 */

#include <rtl.h>
#include <rtl_fifo.h>
#include <time.h>
#include <rtl_sched.h>
#include <rtl_sync.h>
#include <pthread.h>
#include <unistd.h>
#include <rtl_debug.h>
#include <errno.h>
#include <semaphore.h>
#include "common.h"

int irq=14;
int ntests=100;
int setfocus=0; /* 0 or 1 */

MODULE_PARM(irq,"i");
MODULE_PARM(ntests,"i");
MODULE_PARM(setfocus,"i");

pthread_t thread;
int fd_fifo;
hrtime_t irqtime;
sem_t irqsem;

unsigned int intr_handler(unsigned int irq, struct pt_regs *regs)
{
/* 	rtl_printf("%d", rtl_getcpuid()); */
	irqtime = gethrtime();
	rtl_global_pend_irq (irq);
	sem_post(&irqsem);
	return 0;
}


void *thread_code(void *param) {

	hrtime_t diff;
	hrtime_t now;
	hrtime_t min_diff;
	hrtime_t max_diff;
	struct sample samp;
	int i;
	int cpu_id = rtl_getcpuid();

	rtl_printf ("Measurement task starts on CPU %d\n", cpu_id);

	fd_fifo = open("/dev/rtf0", O_NONBLOCK);
	if (fd_fifo < 0) {
		rtl_printf("/dev/rtf0 open returned %d\n", fd_fifo);
		return (void *) -1;
	}

	do {
		min_diff = 2000000000;
		max_diff = -2000000000;

		for (i = 0; i < ntests; i++) {
			sem_wait (&irqsem);
			now = gethrtime();

			diff = now - irqtime;
			if (diff < min_diff) {
				min_diff = diff;
			}
			if (diff > max_diff) {
				max_diff = diff;
			}
		}

		samp.min = min_diff;
		samp.max = max_diff;
		write (fd_fifo, &samp, sizeof(samp));
	} while (1);
	return 0;
}


unsigned long oldaffinity;

int init_module(void)
{
	int ret;
	unsigned long affinity = 1 << rtl_getcpuid();

	rtf_destroy(0);
	rtf_create(0, 4000);

	ret = rtl_request_irq (irq, intr_handler);
	if (ret) {
		rtl_printf("failed to get irq%d: %d\n", irq, ret);
	} else {
		rtl_printf("got irq%d\n", irq);
	}

	if (setfocus) {
		rtl_printf("setting irq%d distribution mask to %x\n", irq, affinity);
		rtl_irq_set_affinity (irq, &affinity, &oldaffinity);
	}
	sem_init (&irqsem, 1, 0);
	rtl_printf("RTLinux measurement module on CPU %d\n",rtl_getcpuid());
	pthread_create (&thread,  NULL, thread_code, (void *)1);

	return 0;
}


void cleanup_module(void)
{
        rtl_printf ("Removing module on CPU %d\n", rtl_getcpuid());

	rtl_free_irq (irq);
	if (setfocus) {
		rtl_irq_set_affinity (irq, &oldaffinity, NULL);
	}
	rtl_printf("freed irq%d\n", irq);

	pthread_cancel (thread);
	pthread_join (thread, NULL);
	sem_destroy (&irqsem);
	close(fd_fifo);
	rtf_destroy(0);
}

