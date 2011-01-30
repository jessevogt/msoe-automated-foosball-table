/*
 * Written by Der Herr Hofrat, der.herr@hofr.at
 * (C) 2002 FSMLabs
 * License: GPL Version 2
 */
#include <rtl.h>
#include <time.h>
#include <pthread.h>
#include <rtl_signal.h> /* RTL_SIGNAL_WAKEUP */
#include <linux/sched.h>   /* flush_signals() */
#include <linux/module.h>
#include <linux/version.h>
#include <linux/init.h>

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/mman.h>
#include <linux/slab.h>
#include <linux/wrapper.h>

#include <asm/io.h>
#include <asm/uaccess.h>  

static pthread_t rt_thread;

#define DRIVER_MAJOR 17

#define LEN 4096
static char *kmalloc_area;

static void * 
rtthread_code(void *arg)
{
	struct sched_param p;
	p . sched_priority = 1;
	pthread_setschedparam (pthread_self(), SCHED_FIFO, &p);
	
	pthread_make_periodic_np (pthread_self(), gethrtime(), 500000000);

	while (1) {
		pthread_wait_np();
		rtl_printf("RT-Thread current buffer=%s\n",kmalloc_area);
	}
	return 0;
}

static int 
driver_open(struct inode *inode, 
	struct file *file )
{
	MOD_INC_USE_COUNT;
    return 0;
}

static int 
driver_close(struct inode *inode, 
	struct file *file)
{
	MOD_DEC_USE_COUNT;
    return 0;
}

static int
driver_mmap(struct file *file,
	struct vm_area_struct *vma)
{
	vma->vm_flags |= VM_SHARED|VM_RESERVED;
  
	if(remap_page_range(vma->vm_start,
		virt_to_phys(kmalloc_area),
		LEN,
		PAGE_SHARED))
		{
			printk("mmap failed\n");
			return -ENXIO;
		}
	return 0;
}

	
static struct file_operations simple_fops={
    mmap:	driver_mmap,
	open:	driver_open, 
	release:	driver_close,
};

static int __init simple_init(void)
{
	struct page *page;
	int ret;
	kmalloc_area=kmalloc(LEN,GFP_USER);
	if(!kmalloc_area){
		printk("kmalloc failed - exiting\n");
		return -1;
	}
	page = virt_to_page(kmalloc_area); 
	mem_map_reserve(page);
	memset(kmalloc_area,0,LEN);

    if(register_chrdev(DRIVER_MAJOR,"simple-driver", &simple_fops) == 0) {
        printk("driver for major %d registered successfully\n",DRIVER_MAJOR);
		ret = pthread_create (&rt_thread, 
			NULL, 
			rtthread_code, 
			0);
        return 0;
    }
    printk("unable to get major %d\n",DRIVER_MAJOR);
    return -EIO;
}

static void __exit simple_exit(void)
{
	pthread_delete_np (rt_thread);

    unregister_chrdev(DRIVER_MAJOR,"simple-driver");
	kfree(kmalloc_area);
}

module_init(simple_init);
module_exit(simple_exit);
