/*  PARALLEL
    periodic scheduled task that toggles parallel port on each invocation.

    Originally by Michael "FZ" Barabanov
    Modified by Victor Yodaiken yodaiken@cs.nmt.edu 
    (c) Barabanov,Yodaiken 1997,1998

    */

#include <linux/module.h>
#include <linux/kernel.h>
#include <rtl_fifo.h>
#include <rtl_time.h>
#include <rtl_sched.h>
#include <asm/io.h>
#include <linux/cons.h>
#include "common.h"

#define PERIOD 500000

RT_TASK mytask;
static int output = 0xffff;

void fun(int t) {

	while (1) {
		conpr("H");
		outb(output, LPT_PORT);
		output = ~output;
		rt_task_wait();
	}
}


int init_module(void)
{
        printk("Starting parallel port module\n");
	rt_task_init(&mytask, fun, 1, 3000, 4);
	rt_task_make_periodic(&mytask, rt_get_time(), PERIOD);
	return 0;
}


void cleanup_module(void)
{
	rt_task_delete(&mytask);

}

