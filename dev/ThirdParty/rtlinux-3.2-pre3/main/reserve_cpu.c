/*
 * (C) Finite State Machine Labs Inc. 1999 <business@fsmlabs.com>
 *
 * Released under the terms of GPL 2.
 * Open RTLinux makes use of a patented process described in
 * US Patent 5,995,745. Use of this process is governed
 * by the Open RTLinux Patent License which can be obtained from
 * www.fsmlabs.com/PATENT or by sending email to
 * licensequestions@fsmlabs.com
 */

#include <linux/version.h>
#include <linux/config.h>
#include <rtl_conf.h>

#ifdef CONFIG_RTL_SUSPEND_LINUX

#ifndef CONFIG_SMP
#error CONFIG_RTL_SUSPEND_LINUX is not supported for non-SMP configurations. Please disable it.
#endif

#if (LINUX_VERSION_CODE < 0x020300)
#error CONFIG_RTL_SUSPEND_LINUX is not supported for 2.2.X kernels. Please disable it.
#endif


#include <reserve_cpu.h>
#include <rtl_core.h>
#include <linux/sched.h>
#include <asm/irq.h>
#include <asm/smplock.h>
#include <linux/smp.h>
#include <rtl_signal.h>
#include <linux/malloc.h>
#define __KERNEL_SYSCALLS__
#include <linux/sched.h>
#include <linux/unistd.h>
#include <asm/uaccess.h>
static int errno;


static int reserve_thread(void * arg);
static void reserve_irq(int irq, void *dev_id, struct pt_regs *p);
static void restore_linux_irq(int irq, void *dev_id, struct pt_regs *p);
static int idle_or_fork(void *);
#define NOLINUX_CLONE	(CLONE_FS | CLONE_FILES | CLONE_SIGHAND | SIGCHLD)

/* global data: better not rmmod until all users of this
 * are gone
 * */

struct reserve_thread_data {
	atomic_t telescoping;
       	atomic_t highwater;
       	atomic_t smp_num_cpus;
	pid_t root;
	int active;
       	int rtlinux_reserve_irq;
       	int rtlinux_restore_irq;
	atomic_t exit;
	wait_queue_head_t wait_reserve;
	wait_queue_head_t wait_telescope;
	wait_queue_head_t wait_root;
	}RD;

/*TODO make these static */
int rtlinux_suspend_linux_signal(void){
	return (RD.rtlinux_reserve_irq?\
		       	RD.rtlinux_reserve_irq+ RTL_LINUX_MIN_SIGNAL:0);
}
int rtlinux_suspend_linux_irq(void) { return RD.rtlinux_reserve_irq;}
int rtlinux_restore_linux_signal(void){
	return (RD.rtlinux_restore_irq?\
		       	RD.rtlinux_restore_irq+ RTL_LINUX_MIN_SIGNAL:0);
}
int rtlinux_restore_linux_irq(void) { return RD.rtlinux_restore_irq;}

/* interrupt handlers */
static void reserve_irq(int irq, void *dev_id, struct pt_regs *p){
	printk("RTLinux got RESERVE irq\n");
	wake_up(&RD.wait_reserve);
	return;
}

/* there can't be more than one of these running at one time can
 * there?
 */
static void restore_linux_irq(int irq, void *dev_id, struct pt_regs *p){
	if(atomic_read(&RD.smp_num_cpus) < atomic_read(&RD.highwater)){
		smp_num_cpus ++;
		atomic_set(&RD.smp_num_cpus,smp_num_cpus);
	}
	return;
}


/* module init */
void rtlinux_suspend_linux_init(void){
	int irq;
	atomic_set(&RD.highwater,smp_num_cpus);
	atomic_set(&RD.smp_num_cpus,smp_num_cpus);
	init_waitqueue_head(&RD.wait_reserve);
	init_waitqueue_head(&RD.wait_telescope);
	init_waitqueue_head(&RD.wait_root);

	irq = rtl_get_soft_irq (reserve_irq, "RTLinux disable linux");
	if (irq < 0) {
		printk("RTLinux RESERVE can't get first interrupt handler\n");
		return;
	}
	RD.rtlinux_reserve_irq = irq;

	irq = rtl_get_soft_irq (restore_linux_irq, "RTLinux restore linux");
	if (irq < 0) {
		printk("RTLinux RESERVE can't get second interrupt handler\n");
		rtl_free_soft_irq(RD.rtlinux_reserve_irq);
		return;
	}

	RD.rtlinux_restore_irq = irq;

	/* start the root kernel thread */
	if ((RD.root= kernel_thread(reserve_thread,0, NOLINUX_CLONE)) < 0){
		rtl_free_irq(RD.rtlinux_reserve_irq);
		rtl_free_irq(RD.rtlinux_restore_irq);
		printk("RTLinux RESERVE fails: can't make initial Kthread\n");
		RD.rtlinux_reserve_irq =0;
		RD.rtlinux_restore_irq =0;
	}
	RD.active = 1;
	return;
}


/* kernel threads */

#define ABSURDLY_HIGH_PRIORITY 1000;
int reserve_thread(void * arg){
       	int target;
       	lock_kernel();
       	exit_files(current);
       	daemonize();
	atomic_set(&RD.telescoping,0);
       	atomic_set(&RD.highwater,smp_num_cpus);
	atomic_set(&RD.exit,0);
       	unlock_kernel();

	/* until the exit flag becomes true
	 * sleep
	 * wakeup to steal a cpu
	 * if still telescoping (stealing the last one) return
	 * else spawn threads until one is on the target CPU and
	 * can choke it.
	 * */
	 
	while(atomic_read(&RD.exit) == 0){
		       	sleep_on(&RD.wait_reserve);
//printk("RTLinux woke up reserve\n");
			if(atomic_read(&RD.exit)){
//printk("RTLinux exit\n");
				if(atomic_read(&RD.telescoping)){
//printk("RTLinux wait telescoping\n");
					sleep_on(&RD.wait_telescope);
//printk("RTLinux woke up telescope exit\n");
				}
				RD.active = 0;
//printk("RTLinux about to wake up root\n");
				wake_up(&RD.wait_root);
				return 0; /* cleanup time */
			}
			if(atomic_read(&RD.telescoping)){
				/* still working on the last one
				 * ignore this*/
				printk("RTLinux RESERVE ignore double call\n");
				continue;
			}

			/* ok now we need to act */
#if LINUX_2_4_0_FINAL_OR_LATER
		       	current->nice =  ABSURDLY_HIGH_PRIORITY;
#else
		       	current->priority =  ABSURDLY_HIGH_PRIORITY;
#endif
		       	current->policy =  SCHED_RR; /* take that */
		       	if(smp_num_cpus > atomic_read(&RD.highwater))
		       	{ /* bad things */
		       	printk("RTLinux RESERVE major sync foulup!\n");
		       	return -1;
		       	}
		       	target = smp_num_cpus -1;
		       	if(target >= 0){
			       	smp_num_cpus --;
				atomic_set(&RD.smp_num_cpus,smp_num_cpus);
			       	atomic_set(&RD.telescoping,1);
				if(kernel_thread(idle_or_fork,(void *)target, NOLINUX_CLONE)< 0){
					printk("RTLinux RESERVE root can't fork");
					smp_num_cpus++;
					atomic_set(&RD.smp_num_cpus,smp_num_cpus);
					return 0;
				}
				else continue;
			}
			else{
				rtl_printf("RTLinux RESERVE no free processors\n");
				continue;
			}
	}
	return 0;
}

extern int rtl_reserved_cpumask;
extern void flush_tlb_all(void);
static int idle_or_fork(void *kv){
	int target = (int) kv;
#if LINUX_2_4_0_FINAL_OR_LATER
       	current->nice =  ABSURDLY_HIGH_PRIORITY;
#else
       	current->priority =  ABSURDLY_HIGH_PRIORITY;
#endif
       	current->policy =  SCHED_RR; /* take that */
       	if(hw_smp_processor_id() == target){
		unsigned long mask[NR_IRQS],oldmask[NR_IRQS],i;
	       	printk("RTLinux RESERVE thread has cpu %d\n",target);
		atomic_set(&RD.telescoping,0);
		local_irq_disable();
		for(i=0; i< NR_IRQS;i++){
		rtl_irq_set_affinity (i, 0, &oldmask[i]);
		mask[i] = oldmask[i];
		clear_bit(hw_smp_processor_id(),&mask[i]);
		rtl_irq_set_affinity (i, &mask[i],0 );
		}
		set_bit (hw_smp_processor_id(), &rtl_reserved_cpumask);
		set_bit (0,&mask);
	       	while((atomic_read(&RD.smp_num_cpus)  <= target)){
			if(atomic_read(&RD.exit)){
					printk("RTLinux RESERVE exits loop\n");
					break;
					}
		}
		for(i=0; i< NR_IRQS;i++){
			rtl_irq_set_affinity (i, &oldmask[i],0);
		}
		local_irq_enable();
		flush_tlb_all();
		printk("RTLinux RESERVE unreserve %d\n",hw_smp_processor_id());
		clear_bit (hw_smp_processor_id(), &rtl_reserved_cpumask);
       	}
       	else{
	       	if(kernel_thread (idle_or_fork,kv, NOLINUX_CLONE)< 0){
		       	printk("RTLinux RESERVE nonroot can't fork\n");
			atomic_set(&RD.telescoping,0);
			smp_num_cpus++;
			atomic_set(&RD.smp_num_cpus,smp_num_cpus);
			return -1;
		}
	       	else{

		       	while(atomic_read(&RD.telescoping)){
				if(atomic_read(&RD.exit)){
					printk("RTLinux RESERVE other exits loop\n");
					break;
				}
			}
			printk("RTLinux RESERVE release %d\n",hw_smp_processor_id());
	       	}
       	}
	return 0;
}

void rtlinux_suspend_linux_cleanup(void){
	lock_kernel();
	if((volatile int)RD.active){
		if(RD.rtlinux_reserve_irq)
			rtl_free_soft_irq(RD.rtlinux_reserve_irq);
		if(RD.rtlinux_restore_irq)
			rtl_free_soft_irq(RD.rtlinux_restore_irq);
		RD.rtlinux_reserve_irq = 0;
		RD.rtlinux_restore_irq = 0;
		atomic_set(&RD.exit,1);
//printk("RTLinux debug reserve about to wake %d\n",RD.root);
		if(RD.root){ wake_up(&RD.wait_reserve); }
		unlock_kernel();
		while((volatile int)RD.active){
			sleep_on_timeout(&RD.wait_root,1000);
//	printk("RTLinux RESERVE cleanup %s\n",(RD.active?"not done":"done"));
		}

	}else  unlock_kernel();
}

#endif
