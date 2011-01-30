/*
 * (C) Finite State Machine Labs Inc. 1995-2000 <business@fsmlabs.com>
 *
 * Released under the terms of GPL 2.
 * Open RTLinux makes use of a patented process described in
 * US Patent 5,995,745. Use of this process is governed
 * by the Open RTLinux Patent License which can be obtained from
 * www.fsmlabs.com/PATENT or by sending email to
 * licensequestions@fsmlabs.com
 */
/*
 * Includes a tiny bit of code from Linux fs/pipe.c copyright (C) Linus Torvalds.
 *
 */
/* ChangeLog
 * Feb 8 2004 Der Herr Hofrat <der.herr@hofr.at>
 *         Macros for fifo status check moved to rtl_fifo.h to be available
 *         for application programmers.
 */


#include <linux/module.h>
#include <linux/major.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/poll.h>
#include <linux/smp.h>
#include <linux/vmalloc.h>

#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/irq.h>

#include <rtl_conf.h>
#include <rtl_fifo.h>
#include <rtl_sync.h>
#include <rtl_core.h>
#include <rtl.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("FSMLabs Inc.");
MODULE_DESCRIPTION("RTLinux FIFOs");

struct rt_fifo_struct rtl_fifos[RTF_MAX_FIFO];
static int rtl_fifo_to_wakeup[RTF_MAX_FIFO] = {0,}; 
static int rtl_fifo_irq = 0; 

#ifdef CONFIG_RTFPREALLOC
#define PREALLOC_BUFFERS CONFIG_NRTFBUFF
#define PREALLOC_SIZE CONFIG_SIZE_RTFBUFF
static int fifo_buffer_control[PREALLOC_BUFFERS]={0,};
static char fifo_buffer[PREALLOC_BUFFERS*PREALLOC_SIZE];
static char * get_prealloc(void)
{
	int i; 
	for(i=0; i < PREALLOC_BUFFERS; i++){
		if(!test_and_set_bit(0,&fifo_buffer_control[i])){
			return &fifo_buffer[i*PREALLOC_SIZE];
		}
	}
		return 0;
}

static int free_prealloc(char *b)
{
	int i;
	for(i = 0; i < PREALLOC_BUFFERS; i++){
		if(b == &fifo_buffer[i*PREALLOC_SIZE])
		{
			clear_bit(0, &fifo_buffer_control[i]);
			return 1;
		}
	}
	return 0;
}

static int find_prealloc(char *b)
{
	int i;
	for(i = 0; i < PREALLOC_BUFFERS; i++){
		if(b == &fifo_buffer[i*PREALLOC_SIZE])
		{
			return 1;
		}
	}
	return 0;
}
#else
#define get_prealloc() 0
#define free_prealloc(x) 0
#define find_prealloc(x) 0
#define  PREALLOC_SIZE 8192 // used for O_CREATE in posix open even if no prealloc
#endif

//#define RTF_ADDR(minor)		(&rtl_fifos[minor])

#define RTF_BI(minor)		(RTF_ADDR(minor)->bidirectional)
#define RTF_ALLOCATED(minor)		(RTF_ADDR(minor)->allocated)
#define RTF_USER_OPEN(minor)	(RTF_ADDR(minor)->user_open)
#define RTF_OPENER(minor)	(RTF_ADDR(minor)->opener)
#define RTF_SPIN(minor)		(RTF_ADDR(minor)->fifo_spinlock)
#define RTF_HANDLER(minor)	(RTF_ADDR(minor)->user_handler)
#define RTF_RT_HANDLER(minor)	(RTF_ADDR(minor)->rt_handler)
#define RTF_PUT_RT_HANDLER(minor)	(RTF_ADDR(minor)->rt_put_handler)
#define RTF_GET_RT_HANDLER(minor)	(RTF_ADDR(minor)->rt_get_handler)
#define RTF_USER_IOCTL(minor)	(RTF_ADDR(minor)->user_ioctl)
#define RTF_WAIT(minor)		(RTF_ADDR(minor)->wait)

#define RTF_WRAP(minor,pos)	((pos) < RTF_BUF(minor)? (pos) : (pos) - RTF_BUF(minor))
#define RTF_MAX_RCHUNK(minor)	(RTF_BUF(minor) - RTF_START(minor))
#define RTF_MAX_WCHUNK(minor)	(RTF_BUF(minor) - RTF_END(minor))

#define RTL_SLEEP_POS 1
#define RTL_NEEDS_WAKE_POS 2

static void rtf_wake_up(void *p)
{
	struct rt_fifo_struct *fifo_ptr = (struct rt_fifo_struct *) p;
	wake_up_interruptible(&(fifo_ptr->wait));
	current->need_resched = 1;
}

static void fifo_wake_sleepers(int );

/* These are for use in the init and exit code of real-time modules
   DO NOT call these from a RT task  */

int rtf_resize(unsigned int minor, int size)
{
	void *mem=0;
	void *old;
	rtl_irqstate_t interrupt_state;

	if (size <= 0) {
		return -EINVAL;
	}

	if (minor >= RTF_MAX_FIFO) {
		return -ENODEV;
	}
	if(size == PREALLOC_SIZE){
		mem=get_prealloc();
	}
	
	if(!mem){
 		if (!rtl_rt_system_is_idle()) {
 			return -EINVAL;
 		}
		mem = vmalloc(size);
	}
	if (!mem) {
		return -ENOMEM;
	}
	memset(mem, 0, size);
	old = RTF_BASE(minor);


	rtl_spin_lock_irqsave(&RTF_SPIN(minor), interrupt_state);
	RTF_BASE(minor) = mem;
	RTF_BUF(minor) = size;
	RTF_START(minor) = 0;
	RTF_LEN(minor) = 0;
	rtl_spin_unlock_irqrestore(&RTF_SPIN(minor), interrupt_state);

	if (RTF_ALLOCATED(minor) && old && !free_prealloc(old)){
		vfree(old);
	}
	return 0;
}

extern int rtf_link_user_ioctl (unsigned int minor,
		int (*handler)(unsigned int fifo, unsigned int cmd, unsigned long arg))
{
	if (minor >= RTF_MAX_FIFO) {
		return -ENODEV;
	}
	RTF_USER_IOCTL(minor) = handler;
	return 0;
}


extern int rtf_make_user_pair (unsigned int fifo_get, unsigned int fifo_put)
{
	if (fifo_get >= RTF_MAX_FIFO || fifo_put >= RTF_MAX_FIFO) {
		return -ENODEV;
	}
	if (!RTF_ALLOCATED(fifo_get) || !RTF_ALLOCATED(fifo_put)) {
		return -EINVAL;
	}
	RTF_BI(fifo_get) = (fifo_put - fifo_get);
	RTF_BI(fifo_put) = -(fifo_put - fifo_get);
	return 0;
}


int __rtf_create(unsigned int minor, int size, struct module *creator)
{
	int ret;

	if (minor >= RTF_MAX_FIFO) {
		return -ENODEV;
	}
	if (RTF_ALLOCATED(minor)) {
		return -EBUSY;
	}
	spin_lock_init(&RTF_SPIN(minor));
	RTF_BI(minor) = 0;
	if ((ret = rtf_resize(minor, size)) < 0) {
		return -ENOMEM;
	}
	RTF_ADDR(minor)->creator = creator;
	RTF_USER_OPEN(minor) = 0;
	RTF_OPENER(minor) = 0;
	RTF_HANDLER(minor) = NULL;
	RTF_RT_HANDLER(minor) = NULL;
	RTF_USER_IOCTL(minor) = 0;

#if LINUX_VERSION_CODE >= 0x020300
	init_waitqueue_head(&RTF_WAIT(minor));
#else
	init_waitqueue (&RTF_WAIT(minor));
#endif
	RTF_ALLOCATED(minor) = 1;
	return 0;
}


int rtf_destroy(unsigned int minor)
{
	if (minor >= RTF_MAX_FIFO) {
		return -ENODEV;
	}
	if (RTF_USER_OPEN(minor)) {
		return -EINVAL;
	}
	if (!RTF_ALLOCATED(minor)) {
		return -EINVAL;
	}
	RTF_ADDR(minor)->creator = 0;
	RTF_HANDLER(minor) = NULL;
	RTF_RT_HANDLER(minor) = NULL;
	if (!free_prealloc(RTF_BASE(minor))){
		vfree(RTF_BASE(minor));
	}
	RTF_ALLOCATED(minor) = 0;
	return 0;
}


int rtf_create_handler(unsigned int minor, int (*handler) (unsigned int fifo))
{
	if (minor >= RTF_MAX_FIFO || !RTF_ALLOCATED(minor) || !handler) {
		return -EINVAL;
	}
	RTF_HANDLER(minor) = handler;
	return 0;
}


extern int rtf_create_rt_handler(unsigned int minor,
		int (*handler)(unsigned int fifo))
{
	if (minor >= RTF_MAX_FIFO || !RTF_ALLOCATED(minor) || !handler) {
		return -EINVAL;
	}
	RTF_RT_HANDLER(minor) = handler;
	return 0;
}


/* these can be called from RT tasks and interrupt handlers */

int rtf_isempty(unsigned int minor)
{
	return RTF_LEN(minor) == 0;
}

int rtf_isused(unsigned int minor)
{
	return RTF_USER_OPEN(minor) != 0;
}

int rtf_flush(unsigned int minor)
{
	rtl_irqstate_t interrupt_state;
	rtl_spin_lock_irqsave(&RTF_SPIN(minor), interrupt_state);
	RTF_LEN(minor) = 0;
	rtl_spin_unlock_irqrestore(&RTF_SPIN(minor), interrupt_state);
	return 0;
}

int rtf_put(unsigned int minor, void *buf, int count)
{
	rtl_irqstate_t interrupt_state;
	int chars = 0, free = 0, written = 0;
	char *pipebuf;

	if (minor >= RTF_MAX_FIFO) {
		return -ENODEV;
	}
	if (!RTF_ALLOCATED(minor))
		return -EINVAL;

	rtl_spin_lock_irqsave(&RTF_SPIN(minor), interrupt_state);
	if (RTF_FREE(minor) < count) {
		rtl_spin_unlock_irqrestore(&RTF_SPIN(minor), interrupt_state);
		return -ENOSPC;
	}
	while (count > 0 && (free = RTF_FREE(minor))) {
		chars = RTF_MAX_WCHUNK(minor);
		if (chars > count)
			chars = count;
		if (chars > free)
			chars = free;
		pipebuf = RTF_BASE(minor) + RTF_END(minor);
		written += chars;
		RTF_LEN(minor) += chars;
		count -= chars;
		memcpy(pipebuf, buf, chars);
		buf += chars;
	}
	rtl_spin_unlock_irqrestore(&RTF_SPIN(minor), interrupt_state);

	if((*RTF_RT_HANDLER(minor))){
		(*RTF_RT_HANDLER(minor))(minor);
	}

	if (RTF_USER_OPEN(minor)) {
		fifo_wake_sleepers(minor - (RTF_BI(minor) < 0));
	}
	return written;
}


int rtf_get(unsigned int minor, void *buf, int count)
{
	rtl_irqstate_t interrupt_state;
	int chars = 0, size = 0, read = 0;
	char *pipebuf;

	if (minor >= RTF_MAX_FIFO) {
		return -ENODEV;
	}
	if (!RTF_ALLOCATED(minor))
		return -EINVAL;

	rtl_spin_lock_irqsave(&RTF_SPIN(minor), interrupt_state);
	while (count > 0 && (size = RTF_LEN(minor))) {
		chars = RTF_MAX_RCHUNK(minor);
		if (chars > count)
			chars = count;
		if (chars > size)
			chars = size;

		read += chars;
		pipebuf = RTF_BASE(minor) + RTF_START(minor);
		RTF_START(minor) += chars;
		RTF_START(minor) = RTF_WRAP(minor, RTF_START(minor));
		RTF_LEN(minor) -= chars;
		count -= chars;
		memcpy(buf, pipebuf, chars);
		buf += chars;
	}
	rtl_spin_unlock_irqrestore(&RTF_SPIN(minor), interrupt_state);

	if(*RTF_RT_HANDLER(minor)){
		(*RTF_RT_HANDLER(minor))(minor);
	}

	if (RTF_USER_OPEN(minor)) {
		fifo_wake_sleepers(minor);
	}
	return read;
}


/* these are RTL-FIFO internal functions */

static int rtf_ioctl (struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	unsigned int minor = MINOR(inode->i_rdev);
	int val;

	if (RTF_USER_IOCTL(minor)) {
		return (*RTF_USER_IOCTL(minor)) (minor, cmd, arg);
	} else {
		switch (cmd) {
			case FIONREAD:
				val = RTF_LEN(minor);
				put_user(val, (int *) arg);
				break;
			default:
				break;
		}
		return 0;
	}
}

static void fifo_setup_sleep(unsigned int minor)
{
/* 	clear_bit(0,&rtl_fifo_to_wakeup[minor]); */
}

static void fifo_wake_sleepers(int minor)
{
	if (waitqueue_active(&RTF_WAIT(minor))) {
		set_bit(0, &rtl_fifo_to_wakeup[minor]);
		if (rtl_fifo_irq > 0) {
			rtl_global_pend_irq(rtl_fifo_irq);
		}
	}
}

static void fifo_irq_handler (int irq, void *dev_id, struct pt_regs *p)
{
	int minor;

	for(minor=0; minor < RTF_MAX_FIFO; minor++) {
		if (test_and_clear_bit(0,&rtl_fifo_to_wakeup[minor])) {
			rtf_wake_up(RTF_ADDR(minor));
		}
	}
}


/* 
 * these are file_operations functions
 * called by user tasks via the fops structure 
 */


static int rtf_open(struct inode *inode, struct file *filp)
{
	unsigned int minor = MINOR(inode->i_rdev);

	if (minor >= RTF_MAX_FIFO)
		return -ENODEV;

	if (!RTF_ALLOCATED(minor)) {
		return -ENODEV;
	}
	if (RTF_USER_OPEN(minor) && current != RTF_OPENER(minor)) {
		return -EACCES;
	}
	RTF_OPENER(minor) = current;
	RTF_USER_OPEN(minor)++;

	if (RTF_ADDR(minor)->creator) {
		__MOD_INC_USE_COUNT(RTF_ADDR(minor)->creator);
	}
	return 0;
}


static int rtf_release(struct inode *inode, struct file *file)
{
	unsigned int minor = MINOR(inode->i_rdev);

#if 0
	if (!RTF_USER_OPEN(minor)) {
		printk("rtf: release on a not opened descriptor inode=%d\n",(int)inode->i_ino);
		return 0;  /* that was just a warning */
	}
#endif
	RTF_USER_OPEN(minor)--;
	if (RTF_ADDR(minor)->creator) {
		__MOD_DEC_USE_COUNT(RTF_ADDR(minor)->creator);
	}
	return 0;
}


static loff_t rtf_llseek(struct file *file, loff_t offset, int origin)
{
	return -ESPIPE;
}

#define RTL_FIFO_TIMEOUT (HZ/10)

static ssize_t rtf_read(struct file *filp, char *buf, size_t count, loff_t* ppos)
{
	struct inode* inode = filp->f_dentry->d_inode;
	unsigned int minor = MINOR(inode->i_rdev);
	rtl_irqstate_t interrupt_state;
	int result;

	int chars = 0, size = 0, read = 0;
	char *pipebuf;

	minor = minor + RTF_BI(minor);

	if (filp->f_flags & O_NONBLOCK) {
		/*      if (RTF_LOCK(minor))
		   return -EAGAIN;
		 */
		if (RTF_EMPTY(minor))
			return 0;
	} else
		while (RTF_EMPTY(minor)  ) {
			if (signal_pending(current))
				return -ERESTARTSYS;
			fifo_setup_sleep(minor);
			interruptible_sleep_on_timeout(&RTF_WAIT(minor),
					RTL_FIFO_TIMEOUT);
		}
/*      RTF_LOCK(minor)++; */
	while (count > 0 && (size = RTF_LEN(minor))) {
		chars = RTF_MAX_RCHUNK(minor);
		if (chars > count)
			chars = count;
		if (chars > size)
			chars = size;

		read += chars;
		pipebuf = RTF_BASE(minor) + RTF_START(minor);
		count -= chars;
		copy_to_user(buf, pipebuf, chars); 
		rtl_spin_lock_irqsave(&RTF_SPIN(minor), interrupt_state);
		RTF_START(minor) += chars;
		RTF_START(minor) = RTF_WRAP(minor, RTF_START(minor));
		RTF_LEN(minor) -= chars;
		rtl_spin_unlock_irqrestore(&RTF_SPIN(minor), interrupt_state);
		buf += chars;
	}
/*      RTF_LOCK(minor)--; */
	if (read) {
		inode->i_atime = CURRENT_TIME;
		if((*RTF_HANDLER(minor))){
			if ((result = (*RTF_HANDLER(minor)) (minor)) < 0) {
				return result;
			}
		}
		return read;
	}
	return 0;
}


static ssize_t rtf_write(struct file *filp, const char *buf, size_t count, loff_t* ppos)
{
	struct inode* inode = filp->f_dentry->d_inode;
	unsigned int minor = MINOR(inode->i_rdev);
	rtl_irqstate_t interrupt_state;
	int chars = 0, free = 0, written = 0;
	char *pipebuf;
	int result;

	if (count <= RTF_BUF(minor))
		free = count;
	else
		free = 1;

	while (count > 0) {
		while (RTF_FREE(minor) < free)  {
			if (signal_pending(current))
				return written ? : -ERESTARTSYS;
			if (filp->f_flags & O_NONBLOCK)
				return written ? : -EAGAIN;
			fifo_setup_sleep(minor);
			interruptible_sleep_on_timeout(&RTF_WAIT(minor),
					RTL_FIFO_TIMEOUT);
		}
		/*      RTF_LOCK(minor)++; */
		while (count > 0 && (free = RTF_FREE(minor))) {
			chars = RTF_MAX_WCHUNK(minor);
			if (chars > count)
				chars = count;
			if (chars > free)
				chars = free;
			rtl_spin_lock_irqsave(&RTF_SPIN(minor), interrupt_state);
			pipebuf = RTF_BASE(minor) + RTF_END(minor);
			rtl_spin_unlock_irqrestore(&RTF_SPIN(minor), interrupt_state);
			count -= chars;
			written += chars;

			copy_from_user(pipebuf, buf, chars);
			rtl_spin_lock_irqsave(&RTF_SPIN(minor), interrupt_state);
			RTF_LEN(minor) += chars;
			rtl_spin_unlock_irqrestore(&RTF_SPIN(minor), interrupt_state);
			buf += chars;
		}
		/*      RTF_LOCK(minor)--; */
		free = 1;
	}
	inode->i_ctime = inode->i_mtime = CURRENT_TIME;
	if((*RTF_HANDLER(minor))){
		if ((result = (*RTF_HANDLER(minor)) (minor)) < 0) {
			return result;
		}
	}
	return written;
}

static	unsigned int rtf_poll(struct file *filp, poll_table *wait)
{
	int ret = 0;
	struct inode* inode = filp->f_dentry->d_inode;
	unsigned int minor = MINOR(inode->i_rdev);
	fifo_setup_sleep(minor);
	poll_wait(filp, &RTF_WAIT(minor), wait);

	if (!RTF_EMPTY(minor + RTF_BI(minor))) {
		ret |= POLLIN | POLLRDNORM;
	}
	if (!RTF_FULL(minor)) {
		ret |= POLLOUT | POLLWRNORM;
	}

	return ret;
}

static struct file_operations rtf_fops =
{
llseek: rtf_llseek,
read: rtf_read,
write: rtf_write,
poll: rtf_poll,
ioctl: rtf_ioctl,
open: rtf_open,
release: rtf_release,
};




int rtf_init (void)
{
	int irq = -1;
	int i;

	if (register_chrdev (RTF_MAJOR, "rtf", &rtf_fops)) {
		printk ("RT-FIFO: unable to get major %d\n", RTF_MAJOR);
		return -EIO;
	}
	for (i = 0; i < RTF_MAX_FIFO; i++) {
		rtl_fifo_to_wakeup[i] = 0;
	}
	irq = rtl_get_soft_irq (fifo_irq_handler, "RTLinux FIFO");
	if (irq > 0) {
		rtl_fifo_irq = irq;
	} else {
		unregister_chrdev (RTF_MAJOR, "rtf");
		printk ("Can't get an irq for rt fifos");
		return -EIO;	/* should have a different return */
	}
	return 0;
}
		
void rtf_uninit(void)
{
	if (rtl_fifo_irq) {
		rtl_free_soft_irq(rtl_fifo_irq);
	}
	unregister_chrdev(RTF_MAJOR, "rtf");
}


#ifdef CONFIG_RTL_POSIX_IO
#include <rtl_posixio.h>
#define RTF_DEFAULT_SIZE PREALLOC_SIZE

static int rtl_rtf_open (struct rtl_file *filp)
{
	if (!(filp->f_flags & O_NONBLOCK)) {
		return -EACCES; /* TODO: implement blocking IO */
	}
	if( (filp->f_flags & O_CREAT) &&  !RTF_ALLOCATED(filp->f_minor))
	{
		/* better be calling from Linux and not RT mode unless
		   there are preallocted fifos still */
		__rtf_create(filp->f_minor, RTF_DEFAULT_SIZE, &__this_module);
	}
	if(!RTF_ALLOCATED(filp->f_minor)){
		return -EUNATCH; // protocol driver not attached
	}
	return 0;
}


static int rtl_rtf_release (struct rtl_file *filp)
{
	int minor = filp->f_minor;
	char *old = RTF_BASE(minor);
	if (RTF_ALLOCATED(minor) && old && find_prealloc(old)) {
		rtf_destroy(minor);
	}
	return 0;
}


static ssize_t rtl_rtf_write(struct rtl_file *filp, const char *buf, size_t count, loff_t* ppos)
{
	return rtf_put(RTL_MINOR_FROM_FILEPTR(filp), (char *) buf, count);
	
}

static ssize_t rtl_rtf_read(struct rtl_file *filp, char *buf, size_t count, loff_t* ppos)
{
	return rtf_get(RTL_MINOR_FROM_FILEPTR(filp), buf, count);
	
}

static int rtl_rtf_ioctl (struct rtl_file *filp, unsigned int req, unsigned long arg)
{
	int minor = RTL_MINOR_FROM_FILEPTR(filp);
	if (!RTF_ALLOCATED(minor)) {
		return -EINVAL;
	}
	if (req == RTF_SETSIZE) {
		if (rtf_resize(minor, arg) < 0) {
			return -EINVAL;
		}
	} else {
		return -EINVAL;
	}
	return 0;
}

static struct rtl_file_operations rtl_fifo_fops = {
       	NULL,
	rtl_rtf_read,
	rtl_rtf_write,
	rtl_rtf_ioctl,
	NULL,
	rtl_rtf_open,
	rtl_rtf_release
};
#endif


#ifdef MODULE


int init_module(void)
{
	int ret;
	ret = rtf_init();
	if (ret < 0) {
		return ret;
	}
#ifdef CONFIG_RTL_POSIX_IO
	if (rtl_register_chrdev (RTF_MAJOR, "rtf", &rtl_fifo_fops)) {
		printk ("RT-FIFO: unable to get RTLinux major %d\n", RTF_MAJOR);
		rtf_uninit();
		return -EIO;
	}
#endif
	return 0;

}


void cleanup_module(void)
{
#ifdef CONFIG_RTL_POSIX_IO
	rtl_unregister_chrdev(RTF_MAJOR, "rtf");
#endif
	rtf_uninit();
}


#endif				/* MODULE */
