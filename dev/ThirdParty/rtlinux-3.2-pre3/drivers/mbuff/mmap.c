/*
 * MBUFF kernel virtual memory driver for Linux
 *  Copyright (C) 1999 Tomasz Motylewski <motyl@stan.chemie.unibas.ch>
 *
 *  The general driver layout borrowed from BIOS FLASH driver:
 * Copyright (C) 1998 Stefan Reinauer <stepan@freiburg.linux.de>
 *
 * routines for vmalloc'ed area remapping from bttv.c linux 2.1 driver.
 *
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/config.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>

#include <kernel_compat.h>

#ifdef LINUX_V22
#include <asm/uaccess.h>
#endif

/* heavy wizardry is here, you do not need to know it :-) */
#include "kvmem.h"

#include "mbuff.h"
#include "mbuff_proc.h"

#define DECR(x)	( --(x) >= 0 ? (x) : ( ((x) = 0) , -1 ) )

static struct mbuff *mbuff_list;
char mbuff_default_name[]="default";
static int __shm_deallocate(struct mbuff *mbuff);


#define	MBUFF_WRITE		1		/* opened for writing (exclusive) */
#define	MBUFF_EXCL		2		/* opened with O_EXCL */

/* adr below is the virtual address in user space (usually vma->vm_start) */
/* see mm/mmap.c for these function calls*/

/* Unfortunately all the functions in vm_operations_struct are passed only
vm_area_struct argument, which does not contain any field which could be used
for identification which memory buffer was mapped by which device. 
So automatic allocation on mmap /deallocation on unmap are not easy for
multiple buffers. Some index could be passed in vm_offset field, but this is 
ugly. Much better would be generic area->priv pointer like in dev->priv.
*/

#if 0
spinlock_t mbuff_list_spinlock;
#endif

static void mbuff_list_add(struct mbuff *mbuff)
{
	mbuff->prev=NULL;
	
	/* grab write spinlock */
	/* see note with mbuff_list_lookup_name() */

	mbuff->next=mbuff_list;
	if(mbuff->next)
		(mbuff->next)->prev=mbuff;

	mbuff_list=mbuff;

        proc_mbuff_add_mbuff(mbuff);

	/* drop spinlock */
}

static void mbuff_list_remove(struct mbuff *mbuff)
{
	struct mbuff *next;
	struct mbuff *prev;

	/* grab write spinlock */

	next=mbuff->next;
	prev=mbuff->prev;

	if(next)next->prev=prev;
	if(prev)prev->next=next;
	else mbuff_list=next;
	
	/* drop spinlock */

	mbuff->next=NULL;
	mbuff->prev=NULL;

        proc_mbuff_remove_mbuff(mbuff);
}

static struct mbuff * mbuff_list_remove_vma(struct vm_area_struct *area)
{
	int i;
	struct mbuff *mbuff;

	/* get read spinlock */

	mbuff=mbuff_list;

	for(;mbuff!=NULL; mbuff= mbuff->next) 
		for(i=0;i<MBUFF_MAX_MMAPS;i++) 
			if(mbuff->vm_area[i]==area)
				goto found;
	/* drop spinlock */
	return NULL;
	found:
	mbuff->vm_area[i]=NULL;
	__shm_deallocate(mbuff); /* this does not decrement counters now */
	MOD_DEC_USE_COUNT;
	/* drop spinlock */
	return mbuff;
}
int mbuff_list_add_vma(struct mbuff * mbuff, struct vm_area_struct *area) {
	int i;
	for(i=0;i<MBUFF_MAX_MMAPS;i++) 
		if(mbuff->vm_area[i] == NULL) {
			mbuff->vm_area[i]=area;
			MOD_INC_USE_COUNT;
// this is mmap			mbuff->count++;
			break;
		}
	if(i==MBUFF_MAX_MMAPS) 
		return -ENOMEM;
	else
		return 0;
}
		

struct mbuff * mbuff_list_lookup_file(struct file *file)
{
	struct mbuff *mbuff;

	mbuff = (struct mbuff *) (file->private_data);
	if(mbuff==NULL)
		printk(KERN_WARNING "mbuff_list_lookup_file: NULL private on %p\n",file);

	return mbuff;
}

/*
   Be careful here!  If priority is non-zero, it means we're calling
   from a priority that is potentially above spinlocks.  This means
   that even if you hold the write spinlock, you need to write the
   mbuff list in a particular order, so that it stays in a consistent
   state so that it can be read here.
 */
struct mbuff * mbuff_list_lookup_name(const char *name,int priority)
{
	struct mbuff *mbuff;

	/* get read spinlock */

	mbuff=mbuff_list;

	for( ; mbuff!=NULL; mbuff=mbuff->next){
		if(!strncmp(mbuff->name,name,MBUFF_NAME_LEN))
			break;
	}

	/* drop spinlock */

	return mbuff;
}

struct mbuff * mbuff_list_lookup_buf(void *buf)
{
	struct mbuff *mbuff;

	/* get read spinlock */

	mbuff=mbuff_list;

	for( ; mbuff!=NULL; mbuff=mbuff->next){
		if(mbuff->buf==buf)
			break;
	}

	/* drop spinlock */

	return mbuff;
}

static void mb_close(struct vm_area_struct * area) {
	struct mbuff *mbuff;

	PRINT("unmapping vma %p\n",area);
	mbuff=mbuff_list_remove_vma(area);

	if(!mbuff){
		printk(KERN_ERR "closing unknown mbuff %p\n",area);
		return;
	}
}

struct vm_operations_struct mbuff_vm_op ={
	close: mb_close,
	} ;

static int mbuff_allocate(const char *name, struct mbuff ** mb) {
	(*mb)=kmalloc(sizeof(struct mbuff),GFP_KERNEL);
	if(!(*mb)) return -ENOMEM;
	memset(*mb,0,sizeof(struct mbuff));

	strncpy((*mb)->name,name,MBUFF_NAME_LEN);
	(*mb)->name[MBUFF_NAME_LEN-1]=0;
	mbuff_list_add(*mb);
	return 0;
}

static void mbuff_deallocate(struct mbuff *mbuff) {
	mbuff_list_remove(mbuff);
	kfree(mbuff);
}

/* WARNING: this calls vmalloc, may need to swap out some memory
   do not call from real time nor interrupt nor timer context.
   This code may call schedule() !  */
/* it should be safe to call it from RT-FIFO handler */
int shm_allocate(const char *name,unsigned int size, void **shm) {

	struct mbuff *mbuff;
	int priority=0;
	int re;

/* since the vmalloc can not be run from RT, it makes no sense making
other parts of this driver RT safe. At least not now.
#ifdef __RT__
	if(!is_rtidle())priority=1;
#endif
*/

	mbuff=mbuff_list_lookup_name(name,priority);
	if(mbuff){
		/* We found a pre-existing mbuff with this
		   name. Act appropriately.  */

		if(size>mbuff->size)
			return -EIO; /* possibly EFAULT */

	}else{
		if(size==0)
			return -EIO;
		
		if(priority)
			return -ENOMEM;
		re=mbuff_allocate(name,&mbuff);
		if(re) return re;

		size=((size-1)&PAGE_MASK)+PAGE_SIZE;
		mbuff->buf = (char*) rvmalloc(size);
		if(!mbuff->buf){
			mbuff_deallocate(mbuff);
			return(-ENOMEM);
		}
		PRINT("allocated %d bytes at %p for %p(%.32s)\n",size,
			mbuff->buf, mbuff, mbuff->name);
		mbuff->size=size;
	}

	MOD_INC_USE_COUNT;
	if(shm) {
		*shm=mbuff->buf;
		mbuff->kcount++;
	} else {
		mbuff->count++;
	}

/* controversial : should we return size on success (<0 on failure) or 0 */
	return mbuff->size;
}

static int __shm_deallocate(struct mbuff *mbuff) {
	int i;
// sorry, not here, we do not know who has called this	mbuff->count--;

	if(mbuff->count<=0) {
		for(i=0;(i<MBUFF_MAX_MMAPS) && (mbuff->vm_area[i] == NULL);i++);
		if(i==MBUFF_MAX_MMAPS && mbuff->kcount <= 0) {
			PRINT("deallocating kernel buffer at %p(%.32s), count=%d.\n",mbuff->buf,mbuff->name,mbuff->count);
			rvfree(mbuff->buf,mbuff->size);
			mbuff_deallocate(mbuff);
			return 0;
		} 
		else
			return -EBUSY;
	}
	return mbuff->count;
}

int shm_deallocate(void *shm)
{
	struct mbuff *mbuff;

	mbuff=mbuff_list_lookup_buf(shm);
/* may be unmark kernel mbuff->vm_area[???] here ?*/
	if(!mbuff)
		return -EINVAL;
	if(DECR(mbuff->kcount)>=0) 
		MOD_DEC_USE_COUNT;
	return __shm_deallocate(mbuff);
}

#ifdef LINUX_V22
int mbuff_mmap(struct file *file, struct vm_area_struct *vma) {
#else
int mbuff_mmap(struct inode *inode, struct file *file, struct vm_area_struct *vma) {
#endif
	int re; 
	char *mbuff_0;
	struct mbuff *mbuff;

	mbuff=mbuff_list_lookup_file(file);
	if(!mbuff){
		printk(KERN_WARNING "mbuff_mmap:no buffer selected for this file:%p\n",file);
		return -EINVAL;
	}
	mbuff_0=mbuff->buf;

	if(!mbuff->buf) {
		printk(KERN_ERR "Shared memory buffer has to be allocated before mmap\n");
		return -EAGAIN;
/* it is safer to not support automatic allocation here */
#if 0
		mbuff->size = vma->vm_end - vma->vm_start + vma->vm_offset;
		mbuff->buf = (char*) rvmalloc(mbuff->size);
		if(!mbuff->buf)
			return(-ENOMEM);
#endif
	}
	if((re=rvmmap(mbuff->buf, mbuff->size, vma)) < 0) {
		if(mbuff_0==NULL) {
			rvfree(mbuff->buf, mbuff->size);
			mbuff->buf=NULL; mbuff->size=0; 
		}
		return re;
	}
	if(!vma->vm_ops) {
		vma->vm_ops=&mbuff_vm_op;
	}
#ifdef LINUX_V22
	vma->vm_file=file;
	PRINT("mbuff_mmap success, count=%d\n",file->f_count);
#else
	vma->vm_inode = inode;
	inode->i_count++;
	PRINT("mbuff_mmap \"%.32s\" success, mcount=%d, icount=%ld, inode=%p, file=%p\n",mbuff->name,mbuff->count,inode->i_count,inode,file);
	PRINT("vma:%p, next:%p, next_share:%p, prev_share:%p\n", vma,
		vma->vm_next, vma->vm_next_share, vma->vm_prev_share);
#endif
	re=mbuff_list_add_vma(mbuff,vma);
	if(re) return re;
	return 0;
}

#ifndef MBUFF_LIB_ONLY

#ifdef LINUX_V22
static loff_t mbuff_llseek(struct file *file,loff_t offset,int origin)
#else
static int  mbuff_llseek(struct inode * inode, struct file *file,off_t offset, int origin )
#endif
{
	struct mbuff *mbuff;

	mbuff=mbuff_list_lookup_file(file);
	switch(origin) {
	  case 0:	/* nothing to do */
		break;
	  case 1:
		offset += file->f_pos;
		break;
	  case 2:
		offset += mbuff->size;
		break;
	  default:
		return -EINVAL;
	}
	if(offset>=0 && offset<mbuff->size){
		file->f_pos=offset;
		return 0;
	}else{
		return -EINVAL;
	}
}

#ifdef LINUX_V22
ssize_t mbuff_read(struct file *file, char *buf, size_t count, loff_t *offset)
#else
int mbuff_read(struct inode * inode, struct file *file, char *buf, int count)
#endif
{
	unsigned int i = file->f_pos;
	int m;
	struct mbuff *mbuff;

	mbuff=mbuff_list_lookup_file(file);

	if(!mbuff || !mbuff->buf) 
		return -EAGAIN;
	if(i>=mbuff->size)
		return 0;
	if(i+count >mbuff->size)
		count=mbuff->size-i;
	m=copy_to_user(buf, mbuff->buf+i, count);
	if(m==count) 
		return -EFAULT;
	i-= count -m;
	file->f_pos = i;
	return(count-m);
}

#endif /* MBUFF_LIB_ONLY */


int mbuff_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	struct mbuff_request_struct req;
	struct mbuff * mbuff;
	int re;
	/* I didn't write anything here, cuz I wanted to get it back
	   to you quickly. */
	/* don't forget to check O_EXECL, etc flags */
	if(cmd > IOCTL_MBUFF_LAST)
		return -EINVAL;
	if(copy_from_user(&req, (void*)arg, sizeof(req))) 
		return -EFAULT;
	PRINT("mbuff_ioctl: name:%.32s, size:%d, cmd:%d, file:%p\n",req.name, req.size, cmd, file);
	switch(cmd){
	case IOCTL_MBUFF_ALLOCATE:
		if(file->private_data)
			((struct mbuff*)(file->private_data))->open_cnt--;
		re=shm_allocate(req.name, req.size, NULL);
		if(re<0) return re;
		file->private_data=(void*)(mbuff=mbuff_list_lookup_name(req.name,0));
		mbuff->open_cnt++;
		return mbuff->size;
	case IOCTL_MBUFF_DEALLOCATE:
		mbuff = mbuff_list_lookup_name(req.name,0);
		if(!mbuff)
			return -EINVAL;
		if(DECR(mbuff->count)>=0)
			MOD_DEC_USE_COUNT;
		if(__shm_deallocate(mbuff)==0) {
			file->private_data=(void*)mbuff_list_lookup_name(mbuff_default_name,0);
			return 0;
		}
		return mbuff->size ;
		break;
	case IOCTL_MBUFF_SELECT:
		mbuff = mbuff_list_lookup_name(req.name,0);
		if(!mbuff)
			return -EINVAL;
		if(file->private_data)
			((struct mbuff*)(file->private_data))->open_cnt--;
		file->private_data=(void*)mbuff;
		mbuff->open_cnt++;
		return mbuff->size;
		break;
	case IOCTL_MBUFF_INFO:
	/* could return info about usage counters, mmaps, etc */
		return -ENOSYS;
	default:
		return -EINVAL;
	}
	return -EINVAL;
}

int mbuff_open_with_name( struct inode *inode, struct file *file,  const char * name) 
{
	struct mbuff *mbuff;
	if((mbuff=mbuff_list_lookup_name(name,0))) {
		if ((mbuff->open_cnt && (file->f_flags & O_EXCL)) ||
			(mbuff->open_mode & MBUFF_EXCL))
			return -EBUSY;
		if (file->f_flags & O_EXCL)
			mbuff->open_mode |= MBUFF_EXCL;
		if (file->f_mode & 2)
			mbuff->open_mode |= MBUFF_WRITE;
		mbuff->open_cnt++;
		file->private_data=mbuff;
	}
	return 0;
}

#ifndef MBUFF_LIB_ONLY

static int mbuff_open(struct inode *inode, struct file *file)
{
	int re;
	re = mbuff_open_with_name(inode, file, mbuff_default_name);
	if(re)
		return re;
	MOD_INC_USE_COUNT;
	return 0;
}

/* it looks like mbuff_close() is delayed in case memory is still mapped,
   at least in 2.2.15pre17. So this is called after the last vma in the
   process is closed. CHECKME: what about other kernel versions? */
#ifdef LINUX_V22
static int mbuff_close(struct inode *inode, struct file *file)
#else
static void mbuff_close(struct inode *inode, struct file *file)
#endif
{
	if(file->private_data) {
//FIXME
		((struct mbuff *)(file->private_data))->open_cnt--;
// this open_cnt stuff has no sense. There are so many other ways to operate.
	}
	MOD_DEC_USE_COUNT;
	PRINT("closing file %p mbuff (%s)\n",file,(file->private_data)?((struct mbuff *)(file->private_data))->name:"(NULL)");
#ifdef LINUX_V22
	return 0;
#else
	return ;
#endif
}


static struct file_operations mbuff_fops = {
#ifdef LINUX_V22
	llseek: mbuff_llseek,
#else
	lseek: mbuff_llseek,
#endif
	read: mbuff_read,
	ioctl: mbuff_ioctl,
	mmap: mbuff_mmap,
	open: mbuff_open,
	release: mbuff_close
};

static struct miscdevice rtl_shm_dev = {
	RTL_SHM_MISC_MINOR,
	"rtl_shm",
	&mbuff_fops,
	NULL,
	NULL };
#endif /* MBUFF_LIB_ONLY */

/*
 * ******************************************
 *
 *	module handling
 *
 * ****************************************** 
 */
#ifdef SHM_DEMO
void * demo_buff;
#endif


#ifdef MODULE
int init_module (void)
{
	printk(KERN_INFO "mbuff: kernel shared memory driver v%s for Linux %s\n", MBUFF_VERSION,UTS_RELEASE);
	printk(KERN_INFO "mbuff: (C) Tomasz Motylewski et al., GPL\n");

        proc_mbuff_create();

#ifndef MBUFF_LIB_ONLY
	if (misc_register(&rtl_shm_dev) < 0) {
		printk(KERN_WARNING "mbuff: Could not register mbuff devices. Bye.\n");
		return -EBUSY;
	}
	printk(KERN_INFO "mbuff: registered as MISC device minor %d\n",
		rtl_shm_dev.minor);
#endif

#ifdef SHM_DEMO
	shm_allocate(mbuff_default_name, 1024*1024, &demo_buff);
	MOD_DEC_USE_COUNT; /* compensation of INC in shm_allocate */
#endif
	return 0 ;
}

void cleanup_module (void)
{
	printk("unloading mbuff\n");

        proc_mbuff_destroy();

#ifdef SHM_DEMO
	MOD_INC_USE_COUNT; /* compensation of DEC in shm_deallocate */
	shm_deallocate(demo_buff);
#endif
#ifndef MBUFF_LIB_ONLY
	misc_deregister(&rtl_shm_dev);
#endif
	printk("mbuff device deregistered\n");
}
#endif

