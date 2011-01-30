/* Produce a rectangular wave on output 0 of a parallel port */


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/cons.h>
#include <asm/io.h>

#include <rtl_sched.h>
#include "common.h"




RT_TASK mytask;
RT_TASK mytask2;

void fun(int t) {
	while(1){
		outb(t, LPT_PORT);		/* write on the parallel port */
		rt_task_wait();
	}
}


int init_module(void)
{
	RTIME now = rt_get_time();

		/* this task will be setting the bit */
	rt_task_init(&mytask, fun, 0xffff, 3000, 4);

		/* this task will be resetting the bit */
	rt_task_init(&mytask2, fun, 0, 3000, 5);


	/* the 2 tasks run periodically with an offset of 200000 time units */
	rt_task_make_periodic(&mytask, now + 3000000, 500000);
	rt_task_make_periodic(&mytask2, now + 3200000, 500000);
	return 0;
}


void cleanup_module(void)
{
	rt_task_delete(&mytask);
	rt_task_delete(&mytask2);
}
