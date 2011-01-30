/*
 * Cleaned up by Der Herr Hofrat, der.herr@hofr.at
 * (C) 2003 OpenTech EDV Research GmbH
 * License: GPL Version 2
 */
/*
 * measurement of scheduling jitter using conditional variables
 *
 * insmod parameters:  cpu0=0 cpu1=0 if you want both sending and receiving 
 *                                   thre ad on the same cpu 
 *                     granularity=# integer for the discretion of the 
 *                                   sched jitter "histogram"
 *                     tracer=1      to turn on tracing
 *
 * Tracepoint 1 is at cond_signal, trace point 2 is at cond_wait. The trace 
 * between these two user events shows the path and cause for delays.
 */

#include <rtl.h>
#include <time.h>
#include <rtl_sched.h>
#include <rtl_sync.h>
#include <pthread.h>
#include <unistd.h>
#include <rtl_debug.h>
#include <errno.h>

#define TRACEON 
#ifdef TRACEON
#include <rtl_tracer.h>
#endif

#ifdef CONFIG_SMP
static int cpu0 = 0;
static int cpu1 = 1;
#else
static int cpu0 = 0;
static int cpu1 = 0;
#endif

static int granularity = 20; /* output histogram has 10us steping */

MODULE_PARM(cpu0,"i");
#ifdef CONFIG_SMP
MODULE_PARM(cpu1,"i");
#endif
MODULE_PARM(granularity,"i");

pthread_t thread1;
pthread_t thread2;
pthread_t thread3;

int processor_id[2]= {0,0};
int nb_processors, nb_measures;

unsigned int graph[2][50];
unsigned int g_idx=0;

pthread_mutex_t mutex;
pthread_cond_t  cond;
hrtime_t start_time, end_time;
int diff_time;

#ifdef TRACEON
int max_diff;
#endif

void *thread_code1(void *param)
{
	pthread_make_periodic_np (
		pthread_self(), 
		gethrtime(), 
		1000000); /* 1 milliseconds */

	rtl_printf("RTLinux thread 1 starts on CPU%d\n", rtl_getcpuid());

	while (1) {
		pthread_wait_np ();

		pthread_mutex_lock(&mutex);
		start_time = clock_gethrtime(CLOCK_REALTIME);
#ifdef TRACEON
		rtl_trace2 (RTL_TRACE_USER, (long)1);
#endif
		pthread_cond_signal(&cond);
		pthread_mutex_unlock(&mutex);
	}
	return 0;
}

void *thread_code2(void *param)
{
	int index;
	int nb_schedule = 0;
	nb_measures = 0;

	rtl_printf("RTLinux thread 2 starts on CPU%d\n", rtl_getcpuid());

	while (1) {
		pthread_mutex_lock(&mutex);
		pthread_cond_wait(&cond,&mutex);
		end_time = clock_gethrtime(CLOCK_REALTIME);
		pthread_mutex_unlock(&mutex);

		diff_time = ((int)(end_time - start_time))/1000; // in us

#ifdef TRACEON
		rtl_trace2 (RTL_TRACE_USER, (long) 2);
		if (diff_time > max_diff) {
			max_diff = diff_time;
			rtl_trace2 (RTL_TRACE_USER, (long) max_diff);
			rtl_trace2 (RTL_TRACE_FINALIZE, 0);
		}
#endif 
		index = diff_time / granularity;
		nb_measures += 1;

		if (index < 50) {
			graph[-g_idx][index] += 1;
		} else {
			rtl_printf("Out of bounds %d us\n", diff_time);
		}
		nb_schedule += 1;
		if (nb_schedule == 2500) {
			pthread_wakeup_np(thread3);
			nb_schedule = 0;
		}
	}
	return 0;
}

void *print_graph(void *param)
{
	unsigned int index;
	int last_idx;


	rtl_printf("RTLinux thread print starts on CPU%d\n", rtl_getcpuid());
	while (1) {
		pthread_wait_np ();
		last_idx=-g_idx; g_idx=~g_idx;
		rtl_printf("Activ buffer %d\n",last_idx);
		for (index = 0; index < 50; index++) {
			if(graph[last_idx][index]){
				rtl_printf("%4dus| %u\n",index*granularity, graph[last_idx][index]);
			}
		}
	}
	return 0;
}

int init_module(void) 
{
	pthread_attr_t attr;
	struct sched_param sched_param;
	int ret;
	int processor_index;

	nb_processors = 0;
#ifdef CONFIG_SMP
	printk("Found processor IDs ");
	for (processor_index=0;processor_index < 2; processor_index++) {
		if (rtl_cpu_exists(processor_index)) {
			processor_id[nb_processors++]=processor_index;
			printk("%d ",processor_index);
		}
	}
	printk("\n");
#endif

	memset(graph[0],0, 50);
	memset(graph[1],0, 50);

	if ((ret = pthread_mutex_init(&mutex, NULL))) {
		printk(KERN_ERR "mutex_init returned %d" , ret);
		return ret;
	}

	if ((ret = pthread_cond_init(&cond, NULL))) {
		printk(KERN_ERR "cond_init returned %d" , ret);
	return ret;
	}


	pthread_attr_init (&attr);
#ifdef CONFIG_SMP
	pthread_attr_setcpu_np(&attr, processor_id[cpu1]);
#endif
	sched_param.sched_priority = 2;
	pthread_attr_setschedparam (&attr, &sched_param);
	printk("About to create thread 2\n");
	ret = pthread_create (&thread2,  &attr, thread_code2, (void *)1);
	if (ret != 0) {
		printk("failed to create RT-thread 2: %d\n", ret);
		return -1;
	} else {
		printk("created RT-thread 2\n");
	}

	pthread_attr_init (&attr);
#ifdef CONFIG_SMP
	pthread_attr_setcpu_np(&attr, processor_id[cpu0]);
#endif
	sched_param.sched_priority = 2;
	pthread_attr_setschedparam (&attr, &sched_param);
	printk("About to create thread 1\n");
	ret = pthread_create (&thread1,  &attr, thread_code1, (void *)1);
	if (ret != 0) {
		printk("failed to create RT-thread 1: %d\n", ret);
		return -1;
	} else {
		printk("created RT-thread 1\n");
	}

	pthread_attr_init (&attr);
#ifdef CONFIG_SMP
	pthread_attr_setcpu_np(&attr, processor_id[cpu1]);
#endif
	sched_param.sched_priority = 1;
	pthread_attr_setschedparam (&attr, &sched_param);
	printk("About to create thread print\n");
	ret = pthread_create (&thread3,  &attr, print_graph, (void *)1);
	if (ret != 0) {
		printk("failed to create RT-thread 3: %d\n", ret);
		return -1;
	} else {
		printk("created RT-thread 3\n");
	}

	return 0;
}

void cleanup_module(void)
{
	printk ("Removing module on CPU %d\n", rtl_getcpuid());
	pthread_cancel (thread1);
	pthread_join (thread1, NULL);
	pthread_cancel (thread2);
	pthread_join (thread2, NULL);
	pthread_cancel (thread3);
	pthread_join (thread3, NULL);
}

