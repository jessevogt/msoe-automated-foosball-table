/* vim: set ts=4: */
/*
 * Copywrite 2002 Der Herr Hofrat
 * License GPL V2
 * Author der.herr@hofr.at
 */
/*
 * sample sysctl usage 
 */ 

#include <linux/module.h> /* module macros etc. */
#include <linux/version.h>
#include <linux/kernel.h> /* printk levels */

#ifndef CONFIG_SYSCTL
	#error "Need CONFIG_SYSCTL set to build this sample properly"
	#ifndef CONFIG_PROC_FS
		#error "Need CONFIG_PROC_FS to build this sample properly"
	#endif /* CONFIG_PROC_FS */
#endif /* CONFIG_SYSCTL */

#include <linux/sysctl.h> /* sysctl stuff */
#include <linux/fs.h> /* filp */

static void simple_sysctl_register(void);
enum {
	DEV_SIMPLE_INFO=1,
	DEV_SIMPLE_DEBUG=2
};

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Der Herr Hofrat <der.herr@hofr.at>");
MODULE_DESCRIPTION("sample usage of sysctl functions");

#define INFO_STR_SIZE 1024
static int debug;

struct simple_sysctl_settings {
	char	info[INFO_STR_SIZE];	/* general info */
	int	debug;			/* debug level */
} simple_sysctl_settings;

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

	pos = sprintf(msg, "Some infos via sysctl");
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

		printk("received %d",*valp);
	
		/* sanity check - only want 1 or 0. */
		if (*valp)
			*valp = 1;
		else
			*valp = 0;

		switch (ctl->ctl_name) {
			case DEV_SIMPLE_DEBUG: {
				if (valp == &simple_sysctl_settings.debug)
					debug = simple_sysctl_settings.debug;
				break;
			}
		}
		printk(" - setting debug to %d\n",*valp);
	} /* nothing to do if we are not called in write mode */
	return ret;
}

/* files go in /proc/sys/dev/simple as "info" "debug" etc. */
ctl_table simple_table[] = {
	{DEV_SIMPLE_INFO, "info", &simple_sysctl_settings.info, 
		INFO_STR_SIZE, 0444, NULL, &simple_sysctl_info},
	{DEV_SIMPLE_DEBUG, "debug", &simple_sysctl_settings.debug, 
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
	simple_sysctl_settings.debug = debug;

	initialized = 1;
}

static void 
simple_sysctl_unregister(void)
{
	if (simple_sysctl_header)
		unregister_sysctl_table(simple_sysctl_header);
}

static int 
__init simple_dev_init(void)
{
		simple_sysctl_register();
		return 0;
}

static void 
__exit simple_dev_exit(void)
{
	simple_sysctl_unregister();
}

module_init(simple_dev_init);
module_exit(simple_dev_exit);
