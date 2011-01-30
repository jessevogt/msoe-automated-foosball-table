#include <linux/errno.h>
#include <rtl.h>
#include <time.h>

#include <rtl_sched.h>
#include <rtl_fifo.h>
#include <semaphore.h>

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Der Herr Hofrat <der.herr@hofr.at>");
MODULE_DESCRIPTION("Using fifo-handlers with semaphors");

pthread_t rt_thread;
static sem_t fifo_sem;

void *thread_code(void *t)
{
  while (1) {
    rtl_printf("thread woken\n");
    sem_wait(&fifo_sem);
  }
  return 0;
}

int fifo_handler(unsigned int fifo)
{
	int err;
   int msg;

   /* read all data in the fifo */
   while ((err = rtf_get(1, &msg, sizeof(msg))) == sizeof(msg));

   /* signal blocked thread*/
   sem_post(&fifo_sem);
	return 0;
}

int init_module(void)
{
	pthread_attr_t attr;
	struct sched_param sched_param;
	int ret;

	rtf_destroy(1);
 	ret = rtf_create(1, 4000);

   sem_init (&fifo_sem, 1, 1);

	pthread_attr_init (&attr);
	sched_param.sched_priority = 4;
	pthread_attr_setschedparam (&attr, &sched_param);
	ret = pthread_create (&rt_thread,  &attr, thread_code, (void *)1);

	rtf_create_handler(1, fifo_handler); 
	return 0;
}


void cleanup_module(void)
{
	rtf_destroy(1);
	pthread_cancel (rt_thread);
	pthread_join (rt_thread, NULL);
   sem_destroy(&fifo_sem);
}
