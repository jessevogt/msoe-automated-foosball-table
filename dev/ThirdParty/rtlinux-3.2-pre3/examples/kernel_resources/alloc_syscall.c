/* vim: set ts=4: */
/*
 * Copywrite 2002 Der Herr Hofrat
 * License GPL V2
 * Author der.herr@hofr.at
 */
/*
 * dynamik syscall number - passed via /proc interface
 */ 
 
#include <linux/kernel.h> /* printk level */
#include <linux/module.h> /* kernel version etc. */
#include <linux/proc_fs.h> /* create_proc_entry remove_proc_entry */
#include <linux/sys.h>  /* NR_syscalls */

/* don't forget to make it GPL ;) */
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Der Herr Hofrat");
MODULE_DESCRIPTION("Dynamic allocation of a syscall number");

unsigned long sys_ni_syscall;
extern long sys_call_table[NR_syscalls];

/* our new system call - the __NR_name syntax is for estetic reasons only */
unsigned long __NR_newpal_call = 0;

/* syscalls must return int and there first argument is also int */
typedef int(*pal_syscall_t) (int, ...);

/* /proc/pal_sys_nr "file-descriptor" */
struct proc_dir_entry *proc_pal_sys_nr;

/*
 * Reverse walk through the syscall table until we find a system call that
 * is set to the NotImplemented sys_ni_syscall system call
 * Preserving the entry at NR_syscalls - 1 is necessary to allow clean 
 * removal
 */
int 
grab_free_syscall(pal_syscall_t * new_syscall)
{
	sys_ni_syscall = (unsigned long)sys_call_table[255];
	__NR_newpal_call = NR_syscalls-1;

	/* preserve 255 as pointer to sys_ni_syscall ! */
	__NR_newpal_call--;

	while ((sys_call_table[__NR_newpal_call] != 
		(unsigned long) sys_ni_syscall) && (__NR_newpal_call > 0)){
		__NR_newpal_call--;
			printk("checking %lx (%lx)\n",__NR_newpal_call,(unsigned long)sys_call_table[__NR_newpal_call]);
		}

	/* assign the found free syscall number to our new syscall function */
	sys_call_table[__NR_newpal_call] = (unsigned long) new_syscall;

	return __NR_newpal_call;
}

/* /proc/pal_sys_nr read method - just dump the dynamic syscall number in a 
 * human readable manner 
 */
int 
dump_pal_sys_nr(
	char *page,
	char **start, 
	off_t off, 
	int count,
	int *eof, 
	void *data)
{
    int size = 0;
    size+=sprintf(page+size,"PAL syscall Nr:%d\n",(int)__NR_newpal_call);
	return(size);
}

/*
 * our dynamic system call - just printk that all the world knows we
 * are alive...
 */
int
newpal_call(int something)
{
	printk("yup - lets go for an oops\n");
	return 0;
}

/*
 * check for a free system call - if we get one drop it to the proc
 * directory /proc/pal_sys_nr so that user-space can retriev the 
 * number
 */
int 
init_module(void) 
{
	int pal_nr=0;

	pal_nr=grab_free_syscall((pal_syscall_t *)newpal_call);

	/* set up a proc dir-entry in /proc */
    proc_pal_sys_nr = create_proc_entry("pal_sys_nr",
        S_IFREG | S_IWUSR,
        &proc_root);

	/* assign the read method of /proc/pal_sys_nr to dump the number */
    proc_pal_sys_nr->read_proc = dump_pal_sys_nr;

	printk("newpal_call got %d\n",pal_nr);

	return 0;
}

void 
cleanup_module(void) 
{
	/* get the pointer to sys_ni_syscall */
	sys_ni_syscall = sys_call_table[255];

	/* free the dynamic syscall by reassigning it to sys_ni_syscall */
    sys_call_table[__NR_newpal_call] = (unsigned long) sys_ni_syscall;

	/* remove the proc entry */
    remove_proc_entry("pal_sys_nr", &proc_root);

	printk("out of here\n");
}
