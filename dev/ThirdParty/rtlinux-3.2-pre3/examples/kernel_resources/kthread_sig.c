/* vim: set ts=4: */
/*
 * Written by Der Herr Hofrat, der.herr@hofr.at
 * (C) 2003 OpenTech EDV-Research GmbH
 * License: GPL Version 2
 */
/* RT-thread waking up kthread via tasklet as signal delivery mechanism
 * basically the tasklet is used here only because direct
 * waking of the task (kthread) is not rt-safe as all the
 * functions call schedule() sooner or later. the tasklet here is
 * missused as a generic way to cross the rt/non-rt boundary within the kernel
 */
#include <rtl.h>
#include <time.h>
#include <pthread.h>
#include <rtl_signal.h> /* RTL_SIGNAL_WAKEUP */
#include <linux/sched.h>   /* flush_signals() */
#include <linux/init.h>
#include <linux/interrupt.h> /* linux non-rt tasklets */

/* don't forget to make it GPL ;) */
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Der Herr Hofrat");
MODULE_DESCRIPTION("wake kthread from rt-context via tasklet");

static pid_t kthread_id=0;
static wait_queue_head_t wait;
static int rt_thread_state=1; /* got to initialize it to != 0 */
#define ACTIVE 1
#define TERMINATED 0
static int state=ACTIVE;

#define NAME_LEN 16
static pthread_t rt_thread;

/* need the task struct global for the tasklet */
static struct task_struct *kthread=NULL;
void tasklet_function(unsigned long);

char tasklet_data[64];

DECLARE_TASKLET(sig_tasklet,tasklet_function, (unsigned long) &tasklet_data);

/* minimum periodic rt-thread - just schedule the
 * tasklet in every period
 */
static void * 
rtthread_code(void *arg)
{
	struct sched_param p;
	p . sched_priority = 1;
	pthread_setschedparam (pthread_self(), SCHED_FIFO, &p);
    pthread_make_periodic_np (pthread_self(), gethrtime(), 500000000);

	while (1) {
		pthread_wait_np();
        tasklet_hi_schedule(&sig_tasklet);
	}
	return 0;
}

/* tasklet run in Linux kernel context so we can call the scheduler*/
void tasklet_function(unsigned long data){
   /* kick the kthread in non-rt context 
    * note this is directly invoking the scheduler and not using 
    * signals
    */
   wake_up_process(kthread);
}

static int 
kthread_code( void *data )
{
	char thread_name[NAME_LEN];
    struct timeval now;
    kthread=current;

	memset(thread_name,0,NAME_LEN);
    daemonize();

	/* take the address of the rt-thread as the uniq name */
	sprintf(thread_name,"rtl_%lx",(unsigned long)&rt_thread);
    strcpy(kthread->comm, thread_name);

	/* clear all pending signals */
	spin_lock_irq(&kthread->sigmask_lock);
	sigemptyset(&kthread->blocked);
	flush_signals(kthread);
	recalc_sigpending(kthread);
	spin_unlock_irq(&kthread->sigmask_lock);

	/* wait for signals in an endless loop */
	while(1){

    	interruptible_sleep_on(&wait);

		/* if we got a SIGKILL sent to the kthread
         * terminate the rt-thread and exit the loop 
         */
		if(sigtestsetmask(&kthread->pending.signal,
			sigmask(SIGKILL))  ){
			pthread_delete_np(rt_thread);
			break;
		}
		/* else just wake up and do something 
         * currently this is done via a tasklet scheduled by the r-thread
         */
		else{
            /* flush pending signals 
             * any signal other than SIGKILL which is
             * handled above sent to the kthread will
             * just wake it
             */
			spin_lock_irq(&kthread->sigmask_lock);
			sigemptyset(&kthread->blocked);
			flush_signals(kthread);
			recalc_sigpending(kthread);
			spin_unlock_irq(&kthread->sigmask_lock);

            /* do something */
            do_gettimeofday(&now);
            printk("kthread woke up at %ld (s),%ld (us)\n",now.tv_sec,now.tv_usec);
		}
	}

	/* so cleanup module knows when to safely exit */
	state=TERMINATED;
	printk("kthread terminating\n");
    return(0);
}

int
init_module(void)
{
    init_waitqueue_head(&wait);
    kthread_id=kernel_thread(kthread_code, 
		NULL,
    	CLONE_FS|CLONE_FILES|CLONE_SIGHAND );
	printk("kthread launched (pid %d)\n",
		kthread_id);
	rt_thread_state = pthread_create (&rt_thread, 
		NULL, 
		rtthread_code, 
		0);
	return 0;
}

void  
cleanup_module(void)
{
	int ret;

	/* delete the rt-thread */
	pthread_delete_np (rt_thread);

	/* send a term signal to the kthread */
	ret = kill_proc(kthread_id, SIGKILL, 1);
	if (!ret) {
		int count = 10 * HZ;
		/* wait for the kthread to exit befor terminating */
		while (state && --count) {
			current->state = TASK_INTERRUPTIBLE;
			schedule_timeout(1);
		}
	}
	printk("kthread_sig exit\n");
}
