/*
 * rt_com usage example
 *
 * Michael Barabanov <baraban@fsmlabs.com>, 1999
 *  (C) FSMLabs  1999. baraban@fsmlabs.com
 *  Released under the GNU GENERAL PUBLIC LICENSE Version 2, June 1991
 *  Any use of this code must include this notice.
 *
 */

#include <rtl.h>
#include <rtl_fifo.h>

#include <time.h>
#include <pthread.h>
#include <asm/io.h>
#include "rt_com.h"

int period=100000000;
int fifo_size=4000;
pthread_t thread;
int fd_fifo;

void *thread_code(void *t) {

	pthread_make_periodic_np (thread, gethrtime(), period);

	do {
		int n;
		char buf[210];
		pthread_wait_np();
		n = rt_com_read(0, buf, sizeof(buf));
		if (n > 0) {
			buf[n] = 0;
			rtl_printf("%s", buf);
		}
		rt_com_write(0, "test\n", 5);
	} while (1);

	return 0;
}



int init_module(void)
{
	pthread_attr_t attr;
	struct sched_param sched_param;
	int thread_status;

	rt_com_setup(0, 38400, RT_COM_PARITY_NONE, 1, 8);


	pthread_attr_init (&attr);
	pthread_attr_setcpu_np(&attr, 0);
	sched_param.sched_priority = 1;
	pthread_attr_setschedparam (&attr, &sched_param);
	thread_status = pthread_create (&thread,  &attr, thread_code, (void *)1);
	if (thread_status != 0) {
		printk("failed to create RT-thread\n");
		return -1;
	} else {
		printk("created RT-thread\n");
	}

	return 0;
}


void cleanup_module(void)
{
        printk ("Removing module on CPU %d\n", rtl_getcpuid());
	pthread_delete_np (thread);
	rt_com_setup(0, -1, 0, 0, 0);
}

