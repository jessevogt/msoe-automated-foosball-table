/* vim: set ts=4: */
#include <rtl.h>
#include <time.h>
#include <pthread.h>
#include <rtl_signal.h> /* RTL_SIGNAL_WAKEUP */
#include <linux/sched.h>   /* flush_signals() */
#include <linux/init.h>

static pid_t kthread_id=0;
static wait_queue_head_t wait;
static int rt_thread_state=1; /* got to initialize it to != 0 */
#define ACTIVE 1
#define TERMINATED 0
static int state=ACTIVE;

#define NAME_LEN 16
static pthread_t rt_thread;

static void * 
rtthread_code(void *arg)
{
	struct sched_param p;
	p . sched_priority = 1;
	pthread_setschedparam (pthread_self(), SCHED_FIFO, &p);

	while (1) {
		rtl_printf("RT-Thread woke up\n");
		pthread_suspend_np (pthread_self());
	}
	return 0;
}

static int 
kthread_code( void *data )
{
    struct task_struct *kthread=current;
	char thread_name[NAME_LEN];

	memset(thread_name,0,NAME_LEN);
    daemonize();

	/* wait for pthread_create of the finisch so we are in sync */
	while (!rt_thread_state) {
		current->state = TASK_INTERRUPTIBLE;
		schedule_timeout(1);
	}

	/* take the address of the rt-thread as the uniq name */
	sprintf(thread_name,"rtl_%lx",(unsigned long)&rt_thread);
    strcpy(kthread->comm, thread_name);

	/* make it low priority */
    kthread->nice=20;

	/* clear all pending signals */
	spin_lock_irq(&kthread->sigmask_lock);
	sigemptyset(&kthread->blocked);
	flush_signals(kthread);
	recalc_sigpending(kthread);
	spin_unlock_irq(&kthread->sigmask_lock);

	/* wait for signals to pass on in an endless loop */
	while(1){
    	interruptible_sleep_on(&wait);
		/* if we got a SIGKILL terminate the rt-thread and exit the loop */
		if(sigtestsetmask(&kthread->pending.signal,
			sigmask(SIGKILL))  ){
			pthread_delete_np(rt_thread);
//			set_bit(RTL_SIGNAL_CANCEL, 
//				&rt_thread->pending); 
			break;
		}
		/* else send a RTL_SIGNLA_WAKEUP to the rt-thread and sleep on */
		else{
			set_bit(RTL_SIGNAL_WAKEUP, 
				&rt_thread->pending); 
			spin_lock_irq(&kthread->sigmask_lock);
			sigemptyset(&kthread->blocked);
			flush_signals(kthread);
			recalc_sigpending(kthread);
			spin_unlock_irq(&kthread->sigmask_lock);
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
	printk("rt_sig_thread launched (pid %d)\n",
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
	printk("rt_sig_thread exit\n");
}
