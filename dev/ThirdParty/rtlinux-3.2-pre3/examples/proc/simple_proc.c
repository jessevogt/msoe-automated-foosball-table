/* vim: set ts=4: */
/*
 * Copyrite 2002 Der Herr Hofrat
 * License GPL V2
 * Author der.herr@hofr.at
 */
/*
 * trivial /proc/hrtime file example - read-only access to gethrtime().
 */ 
 
#include <rtl.h>
#include <rtl_time.h>
#include <linux/kernel.h>  /* printk level */
#include <linux/module.h>  /* USE_COUNT macros ,versioning, config etc. */
#include <linux/proc_fs.h> /* proc API */

/* don't forget to make it GPL ;) */
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Der Herr Hofrat");
MODULE_DESCRIPTION("simple proc example");

/* /proc/hrtime "file-descriptor" */
struct proc_dir_entry *proc_hrtime;

/* /proc/hrtime read method - just dump the dynamic syscall number in a 
 * human readable manner 
 */
int 
dump_stuff(
	char *page,
	char **start, 
	off_t off, 
	int count,
	int *eof, 
	void *data)
{
    int size = 0;
	MOD_INC_USE_COUNT;
    
	size+=sprintf(page+size,"RT-Time:%llu\n",(unsigned long long)gethrtime());

	MOD_DEC_USE_COUNT;
	return(size);
}

int 
init_module(void) 
{
	/* set up a proc dir-entry in /proc */
	proc_hrtime = create_proc_entry("hrtime",
        S_IFREG | S_IWUSR,
        &proc_root);

	/* assign the read method of /proc/hrtime to dump the number */
	proc_hrtime->read_proc = dump_stuff;
	return 0;
}

void 
cleanup_module(void) 
{
	/* remove the proc entry */
    remove_proc_entry("hrtime", &proc_root);
	printk("out of here\n");
}
