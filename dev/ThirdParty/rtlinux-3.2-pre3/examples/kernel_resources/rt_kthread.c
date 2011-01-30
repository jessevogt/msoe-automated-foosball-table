/*
 * Written by Der Herr Hofrat, der.herr@hofr.at
 * (C) 2002 FSMLabs
 * License: GPL Version 2
 */

#include <rtl.h>
#include <time.h>
#include <pthread.h>

#include <linux/module.h>
#include <linux/version.h>
#include <linux/init.h>
#include <asm/io.h>
#include <asm/uaccess.h>   /* copy_to_user()       */

#define DRIVER_MAJOR 240

static int kthread_id=0;
static wait_queue_head_t wq;
static int kthread_state=1; /* to sync on kthread in cleanup_module */

static pthread_t rt_thread;

static int myint_for_something=0;

void * 
start_routine(void *arg)
{
	struct sched_param p;
	p . sched_priority = 1;
	pthread_setschedparam (pthread_self(), SCHED_FIFO, &p);

	pthread_make_periodic_np (pthread_self(), gethrtime(), 500000000);

	while (1) {
		pthread_wait_np ();
		rtl_printf("I'm here; my arg is %x\n", (unsigned) arg);
		myint_for_something++;
		rtl_printf("Hello sees myint_for_something = %x\n",
			myint_for_something);
	}
	return 0;
}

static int 
kthread_code( void *data )
{
	struct task_struct *kthread=current;
	unsigned long timeout;
	int i;

	daemonize();
	strcpy(kthread->comm, "rt_kthread");
	kthread->nice=20;

	printk("kthread_code startet ...\n");
	for( i=0; i<60; i++ ) {
        	timeout=HZ;
		do {
			timeout=interruptible_sleep_on_timeout(&wq, timeout);
			printk("ktherad_code: woke up ...\n");
		} while( !signal_pending(current) && (timeout>0) );

		if( signal_pending(current) ) {
			printk("signal resetting myint_for_something to 0\n");
			myint_for_something=0;
			break;
		}
	}
	kthread_state=0;
	return 0;
}

static int 
driver_open(struct inode *inode, 
	struct file *file)
{
	printk("driver_open called\n");
	MOD_INC_USE_COUNT;
	return 0;
}

static int 
driver_close(struct inode *inode,
	struct file *file)
{
	printk("driver_close called\n");
	MOD_DEC_USE_COUNT; 
	return 0;
}

static ssize_t 
driver_read(struct file *File,
	char *buffer,
	size_t count,
        loff_t *dont_know)
{
	char msg[32];

	printk("driver_read called\n");
	sprintf(msg,"myint_for_something = %d",myint_for_something);
	if(copy_to_user(buffer,&msg,sizeof(msg))){
		printk("driver_read: failed on copy_to_user\n");
		return -EFAULT;
	}
	return 0;
}

static struct file_operations Fops = {
    THIS_MODULE,
    NULL,  /* llseek */
    driver_read,  /* read */
    NULL,  /* write */
    NULL,  /* readdir */
    NULL,  /* poll */
    NULL,  /* ioctl */
    NULL,  /* mmap */
    driver_open,  /* open */
    NULL,  /* flush */
    driver_close,  /* release */
    NULL,  /* fsync */
    NULL,  /* fasync */
    NULL,  /* lock */
};

static int __init 
kthread_init(void)
{
	int ret;
	if(register_chrdev(DRIVER_MAJOR, "kthread-Driver", &Fops) == 0) {
		init_waitqueue_head(&wq);
		kthread_id=kernel_thread(kthread_code, 
			NULL,
        	        CLONE_FS|CLONE_FILES|CLONE_SIGHAND );
		printk("kthread running with pid %d\n",kthread_id);
		ret=pthread_create (&rt_thread, NULL, start_routine, 0);	
		if(ret){
			printk("pthread create failed\n");
			unregister_chrdev(DRIVER_MAJOR,"kthread-Driver");
			return -1;
			}
		return 0;
	}
	printk("kthread unable to get major %d\n",DRIVER_MAJOR);
	return( -EIO );
}

static void __exit 
kthread_exit(void)
{
	int ret;
	unregister_chrdev(DRIVER_MAJOR,"kthread-Driver");

	/* send a term signal to the kthread */
	ret = kill_proc(
		kthread_id,
		SIGKILL,
		1);
	if (!ret) {
		int count = 10 * HZ;
		/* wait for the kthread to exit befor terminating */
		while (kthread_state && --count) {
			current->state = TASK_INTERRUPTIBLE;
			schedule_timeout(1);
		}
	}
	pthread_delete_np(rt_thread);
}

module_init(kthread_init);
module_exit(kthread_exit);
