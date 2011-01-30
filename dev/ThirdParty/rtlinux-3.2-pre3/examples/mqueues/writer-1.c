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
	int res,ret;
	int count=50;

	memset(data1,'1',1024);

	 pthread_make_periodic_np(pthread_self(), gethrtime(), 500000000);

	while (count>0) {

		res=mq_send(my_queue,data1,100,1);
		if(res<0)
			rtl_printf("[%d] [%d] Error sending, errno: %d\n",pthread_self(),count,errno);
		else rtl_printf("[%d] [%d] Data has been sent\n",pthread_self(),count);

		count --;
		ret = pthread_wait_np();
	}
		ret = pthread_wait_np();
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
		sprintf(queue_name,"queue%d",i);
		my_queue=mq_open(queue_name,O_RDWR);

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
