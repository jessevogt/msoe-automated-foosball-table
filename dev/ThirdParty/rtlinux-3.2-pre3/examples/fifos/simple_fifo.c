/*
 * Copywrite 2002 Der Herr Hofrat
 * License GPL V2
 * Author der.herr@hofr.at
 */
/*
 * simple fifo example - just write the current rt_time
 * to a fifo in a loop
 */

#include <rtl.h>
#include <rtl_fifo.h>
#include <time.h>
#include <rtl_sched.h>
#include <rtl_sync.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

MODULE_AUTHOR("Der Herr Hofrat");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("simple fifo");

#define FIFO_SIZE 4000
#define FIFO_NR 0
pthread_t thread;
int fd;

void *thread_code(void *param)
{
	hrtime_t now;
	int n = 0;

        pthread_make_periodic_np (pthread_self(), gethrtime(), 500000000);

        while (1) {
                pthread_wait_np ();
		now = clock_gethrtime(CLOCK_MONOTONIC);
		n = write (fd, &now, sizeof(now));
		rtl_printf("wrote %d bytes to fifo %d\n",n,FIFO_NR);
	};
	return 0;
}

int init_module(void)
{
	pthread_attr_t attr;
	struct sched_param sched_param;
        int ret;

	rtf_destroy (FIFO_NR);
	ret = rtf_create (FIFO_NR, FIFO_SIZE);
	if (ret) {
		printk ("failed to create fifo (%d)\n",ret);
		return -1; /* returning -1 terminates insmod */
	}

	fd = open ("/dev/rtf0",O_NONBLOCK);
	if (!fd) {
		printk ("failed to open /dev/rtf%d\n",FIFO_NR);
		return -1;
	}

	pthread_attr_init (&attr);
	sched_param.sched_priority = 1;
	pthread_attr_setschedparam (&attr, &sched_param);
	ret = pthread_create (&thread, &attr, thread_code, (void *)0);
	if (ret) {
		printk ("failed to create thread (%d)\n", ret);
		return -1;
	} else {
		printk("thread created sucessfully\n");
	}

	return 0;
}


void cleanup_module(void)
{
        printk ("Removing simple fifo example\n");
	pthread_cancel (thread);
	pthread_join (thread, NULL);
	close (fd);
	rtf_destroy (FIFO_NR);
}
