#include <linux/errno.h>
#include <rtl.h>
#include <time.h>
#include <semaphore.h>

#include <pthread.h>
#include <mqueue.h>

pthread_t task;
mqd_t my_queue;
mode_t my_mode;
struct mq_attr	my_attr;
struct timespec timeout;

char data1[1024];

void *thread_code(void *t)
{
	int res;

	memset(data1,'1',1024);

	 pthread_make_periodic_np(pthread_self(), gethrtime(), 700000000);

	while (1) {
		int ret;

		timeout.tv_sec=3;
		timeout.tv_nsec=1000000000;
		res=mq_timedreceive(my_queue,data1,100,NULL,&timeout);
		if(res<0)
			rtl_printf("[%d] Error receiving, errno: %d\n",pthread_self(),errno);
		else rtl_printf("[%d] Data has been received\n",pthread_self());

		ret = pthread_wait_np();
	}
	return 0;
}

int init_module(void)
{
	pthread_attr_t attr;
	struct sched_param sched_param;
	int ret;
	char queue_name[10];
	int i=0;

		memset(queue_name,'\0',10);
		my_attr.mq_maxmsg=10;
		my_attr.mq_msgsize=100;
		sprintf(queue_name,"queue%d",i);
		my_queue=mq_open(queue_name,O_RDONLY);

		if(my_queue<0){
			rtl_printf("Error opening queue, errno: %d\n",errno);
			return -1;
		}

	pthread_attr_init (&attr);
	sched_param.sched_priority = 4;
	pthread_attr_setschedparam (&attr, &sched_param);
	ret = pthread_create (&task,  &attr, thread_code, (void *)1);

	return 0;
}


void cleanup_module(void)
{
	pthread_cancel (task);
	pthread_join (task, NULL);
}
