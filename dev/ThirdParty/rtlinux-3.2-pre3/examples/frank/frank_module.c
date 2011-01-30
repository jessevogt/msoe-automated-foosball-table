#include <linux/errno.h>
#include <rtl.h>
#include <time.h>

#include <rtl_sched.h>
#include <rtl_fifo.h>
#include "control.h"

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("FSMLabs Inc.");
MODULE_DESCRIPTION("Example usage of rt-FIFOs");

pthread_t tasks[2];

static char *data[] = {"Frank ", "Zappa "};

#define TASK_CONTROL_FIFO_OFFSET 4

void *thread_code(void *t)
{
	int fifo = (int) t;
	int taskno = fifo - 1;
	struct my_msg_struct msg;
	while (1) {
		int ret;
		int err;
		ret = pthread_wait_np();
		if ((err = rtf_get (taskno + TASK_CONTROL_FIFO_OFFSET, &msg, sizeof(msg))) == sizeof(msg)) {
			rtl_printf("Task %d: executing the \"%d\" command to task %d; period %d\n", fifo - 1,  msg.command, msg.task, msg.period);
			switch (msg.command) {
				case START_TASK:
					pthread_make_periodic_np(pthread_self(), gethrtime(), msg.period * 1000);

					break;
				case STOP_TASK:
//					pthread_suspend_np(pthread_self());
					pthread_kill(pthread_self(),RTL_SIGNAL_SUSPEND);
					break;
				default:
					rtl_printf("RTLinux task: bad command\n");
					return 0;
			}
		}
		rtf_put(fifo, data[fifo - 1], 6);
	}
	return 0;
}

int my_handler(unsigned int fifo)
{
	struct my_msg_struct msg;
	int err;

	while ((err = rtf_get(COMMAND_FIFO, &msg, sizeof(msg))) == sizeof(msg)) {
		rtf_put (msg.task + TASK_CONTROL_FIFO_OFFSET, &msg, sizeof(msg));
		rtl_printf("FIFO handler: sending the \"%d\" command to task %d; period %d\n", msg.command,
				msg.task, msg.period);
		pthread_wakeup_np (tasks [msg.task]);
	}
	if (err != 0) {
		return -EINVAL;
	}
	return 0;
}




/* #define DEBUG  */
int init_module(void)
{
	int c[5];
	pthread_attr_t attr;
	struct sched_param sched_param;
	int ret;

	rtf_destroy(1);
	rtf_destroy(2);
	rtf_destroy(3);
	rtf_destroy(4);
	rtf_destroy(5);
 	c[0] = rtf_create(1, 4000);
	c[1] = rtf_create(2, 4000);
	c[2] = rtf_create(3, 200);	/* input control channel */
	c[3] = rtf_create(4, 100);	/* input control channel */
	c[4] = rtf_create(5, 100);	/* input control channel */

	pthread_attr_init (&attr);
	sched_param.sched_priority = 4;
	pthread_attr_setschedparam (&attr, &sched_param);
	ret = pthread_create (&tasks[0],  &attr, thread_code, (void *)1);

	pthread_attr_init (&attr);
	sched_param.sched_priority = 5;
	pthread_attr_setschedparam (&attr, &sched_param);
	ret = pthread_create (&tasks[1],  &attr, thread_code, (void *)2);

	rtf_create_handler(3, &my_handler); 
	return 0;
}


void cleanup_module(void)
{
#ifdef DEBUG
	printk("%d\n", rtf_destroy(1));
	printk("%d\n", rtf_destroy(2));
	printk("%d\n", rtf_destroy(3));
	printk("%d\n", rtf_destroy(4));
	printk("%d\n", rtf_destroy(5));
#else 
	rtf_destroy(1);
	rtf_destroy(2);
	rtf_destroy(3);
	rtf_destroy(4);
	rtf_destroy(5);
#endif
	pthread_cancel (tasks[0]);
	pthread_join (tasks[0], NULL);
	pthread_cancel (tasks[1]);
	pthread_join (tasks[1], NULL);
}
