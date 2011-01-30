/* vim: set ts=4: */
/*
 * Copywrite 2002 Der Herr Hofrat
 * License GPL V2
 * Author der.herr@hofr.at
 */
/* What this does:
 * launch a kthread that copies the src file to kernel space directly (not 
 * using copy_from_user) the file name is passed as kernel parameter.
 */

#include <linux/module.h> /* MODULE_PARM */
#include <linux/kernel.h> /* printk */
#include <linux/slab.h>   /* __get_free_page(),free_page() */
#include <linux/fs.h>     /* struct file,filp_open,filp_close,IS_ERR,PTR_ERR */
#include <asm/uaccess.h>  /* mm_segment_t,get_fs,set_fs,KERNEL_DS */

char *src = NULL;

/* for testing we allow src/dst to be module parameters 
 * basically this makes no sense for real code.
 */
MODULE_PARM(src,"s");

/* file access is as root:root ! 
 * need to open up kernel space for parameters - dangorous !
 * might be better to take init_fs to escape chroot ?? 
 * passes the original uid,gid and fs-context back to calling 
 * function - return 0 for now.
 * TODO: fix chroot cavet (use init_fs context)
 *       namespace ?
 *       signal stuff - if callable from kthreads (or signal handlers could 
 *         run as root.
 *       error handling.
 */
int 
kspace_init(int *uid,int *gid,mm_segment_t *fs)
{
  *uid=current->fsuid;
  *gid=current->fsgid;
  current->fsuid=current->fsgid=0;

  *fs=get_fs();
  set_fs(KERNEL_DS);

  return 0;
}

/* reset fscontext again. up to the app programmer to preserve it
 * and reset it properly... return 0 for now - what to do on error ?
 * TODO: error handling -> what ? panic on failure ;)
 */
int
kspace_releas(int uid, int gid, mm_segment_t fs)
{
  set_fs(fs);
  current->fsuid=uid;
  current->fsgid=gid;
 
  return 0;
}

/* KernelMore - read file (one page at most) on the filesystem from within 
 * kernel space
 * TODO: check for directories - currently if src if of type dir we still
 *         read some garbage.
 *       preserve file permissions instead of setting them here... carfull 
 *         that we don't open up a suid root hole though...
 *       fs context agains (for umaks etc.) mayby we should use init_fs here ?
 *       use src file size for copy instead of while(!eof) ?
 */

int 
kmore(char *data_file)
{
  struct file *fd0;
  int retval;
  char *buffer;
  unsigned long page;

  printk("kmore: reading %s\n",data_file);
  page = __get_free_page(GFP_KERNEL);
  if(page){
    buffer=(char*)page;
    fd0 = filp_open(data_file,O_RDONLY,0);
    if(IS_ERR(fd0)){
      printk("kmore: Error %ld opening %s\n",-PTR_ERR(fd0),data_file);
    }else{
      if(fd0->f_op&&fd0->f_op->read){
        retval=fd0->f_op->read(fd0,buffer,PAGE_SIZE,&fd0->f_pos);
        if(retval<0){
          printk("kmore: Read error %d\n",-retval);
        }
        if(retval>0){
          buffer[retval]='\0';
          printk(KERN_INFO "read %s\n", buffer);
        }
      }
      retval=filp_close(fd0,NULL);
      if(retval){
        printk("kmore: Error %d closing %s\n",-retval,data_file);
      }
      free_page(page);
    }
  }else{
    printk("kmore: Out of memory\n");
  }
  return 0;
}

static int
kthread_code(void *kthread_arg)
{
  /* needed to save kernel context */
  int old_uid,old_gid;
  mm_segment_t old_fs;
  old_uid=old_gid=0;
  old_fs=get_fs(); /* kind of redundant - could be NULL */

  /* kernel space file copy need to save/restore context only if 
   * called from kthreads or init_module - from within kernel context
   * kcp should only need ot setup KERNEL_DS...
   */
  kspace_init(&old_uid,&old_gid,&old_fs);
  kmore(src);
  kspace_releas(old_uid,old_gid,old_fs);
 
  return 0;
}

int 
init_module(void)
{
  pid_t pid;
  pid = kernel_thread(kthread_code, (void*)0, 0);
  if (pid < 0) {
    printk(KERN_ERR "fork failed, errno %d\n", -pid);
    return pid;
  }
  printk("fork ok, pid %d\n",pid);
	
  return 0;
}

void 
cleanup_module(void)
{
  /* no cleanups for now we assume that kthread_code was done 
   * long befor we can type rmmod kmore...
   */
  printk("nothing to do here - exit\n");
}
