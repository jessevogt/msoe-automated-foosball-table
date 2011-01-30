/*
 * Written by Der Herr Hofrat, der.herr@hofr.at
 * (C) 2002 FSMLabs
 * License: GPL Version 2
 */

#include <rtl.h>
#include <time.h>
#include <pthread.h> 
#include <rtl_signal.h>	
#include <linux/sched.h>
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

#include "device_common.h"

#include <semaphore.h>
static sem_t sem;

static pthread_t rt_thread;

static char *kmalloc_area;

static void * 
rtthread_code(void *arg)
{
	struct sched_param p;
	int ret;
	p . sched_priority = 1;
	pthread_setschedparam (pthread_self(), SCHED_FIFO, &p);

	while (1) {
		ret = sem_wait(&sem);
		rtl_printf("RT-Thread woke up with buffer=%s\n",kmalloc_area);
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
	sem_post(&sem);
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

static int __init 
simple_init(void)
{
	struct page *page;
	int ret;
	char msg[]="Hello Mmap'ed World";

	ret=sem_init(&sem, 1, 1);
	if(ret){
		printk("sem_init failed (%d)\n",ret);
		return -1;
	}

	kmalloc_area=kmalloc(LEN,GFP_USER);
	if(!kmalloc_area){
		printk("kmalloc failed - exiting\n");
		sem_destroy(&sem);
		return -ENOMEM;
	}
	page = virt_to_page(kmalloc_area); 
	mem_map_reserve(page);
	memcpy(kmalloc_area,msg,sizeof(msg));

	if(register_chrdev(SIMPLE_MAJOR,DEV_NAME, &simple_fops) == 0) {
        	printk("driver for major %d registered\n",SIMPLE_MAJOR);
		ret = pthread_create (&rt_thread, 
			NULL, 
			rtthread_code, 
			0);
		if(ret){
			printk("pthread_create failed (%d)\n",ret);
			kfree(kmalloc_area);
			sem_destroy(&sem);
			unregister_chrdev(SIMPLE_MAJOR,DEV_NAME);
			return -1;
			}
		return 0;
	}
	printk("unable to get major %d\n",SIMPLE_MAJOR);
	kfree(kmalloc_area);
	sem_destroy(&sem);
	return -EIO;
}

static void __exit 
simple_exit(void)
{
	/* delete the rt-thread */
	pthread_delete_np (rt_thread);

	unregister_chrdev(SIMPLE_MAJOR,"simple-driver");
	kfree(kmalloc_area);
	sem_destroy(&sem);
	printk("rt_sig_mmap2 exit\n");
}

module_init(simple_init);
module_exit(simple_exit);
