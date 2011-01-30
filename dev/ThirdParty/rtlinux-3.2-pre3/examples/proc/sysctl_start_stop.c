#include <rtl.h>
#include <time.h>
#include <rtl_time.h>
#include <pthread.h>

#ifndef CONFIG_SYSCTL
	#error "Need CONFIG_SYSCTL set to build this sample properly"
	#ifndef CONFIG_PROC_FS
		#error "Need CONFIG_PROC_FS to build this sample properly"
	#endif /* CONFIG_PROC_FS */
#endif /* CONFIG_SYSCTL */

#include <linux/sysctl.h> /* sysctl stuff */
#include <linux/fs.h> /* filp */

pthread_t thread;
hrtime_t start_nanosec;

static void simple_sysctl_register(void);
enum {
	DEV_SIMPLE_INFO=1,
	DEV_SIMPLE_DEBUG=2
};

#define INFO_STR_SIZE 1024
static int status=1;

struct simple_sysctl_settings {
	char	info[INFO_STR_SIZE];	/* general info */
	int	status;			/* status level */
} simple_sysctl_settings;
static int loops=0;

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Der Herr Hofrat");
MODULE_DESCRIPTION("simple proc example");

void * start_routine(void *arg)
{
	struct sched_param p;
	hrtime_t elapsed_time,now;
	p . sched_priority = 1;
	pthread_setschedparam (pthread_self(), SCHED_FIFO, &p);

	pthread_make_periodic_np (pthread_self(), gethrtime(), 500000000);

	while (status == 1) {
		pthread_wait_np ();
		now = clock_gethrtime(CLOCK_REALTIME);
		elapsed_time = now - start_nanosec;
		rtl_printf("elapsed_time = %Ld\n",(long long)elapsed_time);
		loops++;
	}
	return (void *)loops;
}

int 
simple_sysctl_info(ctl_table *ctl, 
	int write, 
	struct file * filp,
	void *buffer, 
	size_t *lenp)
{
	int pos;
	char *msg = simple_sysctl_settings.info;
	
	if (!*lenp || (filp->f_pos && !write))
	{
		*lenp = 0;
		return 0;
	}

	pos = sprintf(msg, "RT-thread done with loop %d",loops);
	return proc_dostring(ctl, write, filp, buffer, lenp);
}

/* This "handler" should take care of all sysctl cases you might need 
 * thats why we have this seemingly useless switch in here 
 */
static int 
simple_sysctl_handler(ctl_table *ctl, 
	int write, 
	struct file * filp,
	void *buffer, 
	size_t *lenp)
{
	int *valp = ctl->data;
	int val = *valp;
	int ret;

	/* grab data if it is int */	
	ret = proc_dointvec(ctl, write, filp, buffer, lenp);


	/* sanity check - is this write ? did we get anything ? */
	if (write && *valp != val) {

		/* sanity check - only want 1 or 0. */
		if (*valp)
			*valp = 1;
		else
			*valp = 0;

		switch (ctl->ctl_name) {
			case DEV_SIMPLE_DEBUG: {
				if (valp == &simple_sysctl_settings.status)
					status = simple_sysctl_settings.status;
				break;
			}
		}
		printk("Terminating thread - setting status to %d\n",*valp);
	} /* nothing to do if we are not called in write mode */
	return ret;
}

/* files go in /proc/sys/dev/simple as "info" "status" etc. */
ctl_table simple_table[] = {
	{DEV_SIMPLE_INFO, "info", &simple_sysctl_settings.info, 
		INFO_STR_SIZE, 0444, NULL, &simple_sysctl_info},
	{DEV_SIMPLE_DEBUG, "status", &simple_sysctl_settings.status, 
		sizeof(int), 0644, NULL, &simple_sysctl_handler},
	{0}};

/* setup a simple subdir */
ctl_table simple_simple_table[] = {
	{DEV_SIMPLE_INFO, "simple", NULL, 0, 0555, simple_table},
	{0}};

/* Make sure that /proc/sys/dev is setup */
ctl_table simple_root_table[] = {
	{CTL_DEV, "dev", NULL, 0, 0555, simple_simple_table},
	{0}};

static struct ctl_table_header *simple_sysctl_header;

static void 
simple_sysctl_register(void)
{
	static int initialized;

	if (initialized == 1)
		return;

	simple_sysctl_header = register_sysctl_table(simple_root_table, 1);
	simple_sysctl_settings.status = status;

	initialized = 1;
}

static void 
simple_sysctl_unregister(void)
{
	if (simple_sysctl_header)
		unregister_sysctl_table(simple_sysctl_header);
}

int 
init_module(void) 
{
	int retval;
	start_nanosec = clock_gethrtime(CLOCK_REALTIME);
	retval = pthread_create (&thread, NULL, start_routine, 0);
	if(retval){
		printk("pthread create failed\n");
		return -1;
	}
	simple_sysctl_register();

	return 0;
}

void 
cleanup_module(void) 
{
	void * ret_val;
	pthread_cancel(thread);
	pthread_join(thread,&ret_val);
	printk("Thread terminated with %d\n",(int)ret_val);
	simple_sysctl_unregister();
}
