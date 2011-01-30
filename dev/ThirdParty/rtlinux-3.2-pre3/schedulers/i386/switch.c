#include <linux/smp.h>
#include <asm/system.h>

#include <rtl_conf.h>
#include <rtl_sched.h>
#include <rtl_core.h>

#ifdef CONFIG_RTL_FP_SUPPORT
int is_set_TS(void)
{
	int current_TS;
	__asm__("movl %%cr0,%%eax;andl $8,%%eax;movl %%eax,%0" \
			:"=m" (current_TS)/*output*/ \
			: /* no inputs */ \
			:"ax");
	return current_TS;
}


void rtl_fpu_save (schedule_t *s, RTL_THREAD_STRUCT *current_t)
{
	if (current_t == &(s->rtl_linux_task)) {
		s->sched_user[0] = is_set_TS();
		clts();
	}
	__asm__("fnsave %0\n\tfwait" : "=m" (current_t->fpu_regs));
}


void rtl_fpu_restore (schedule_t *s,RTL_THREAD_STRUCT *current_t) {
	clts (); /* just in case */
	if (current_t->fpu_initialized) {
		__asm__("frstor %0" : "=m" (current_t->fpu_regs));
	} else {
		__asm__("fninit");
		current_t->fpu_initialized = 1;
	}

	if ((current_t == &(s->rtl_linux_task)) && s->sched_user[0])
	{
		stts();
	}
}


void rtl_task_init_fpu (RTL_THREAD_STRUCT *t, RTL_THREAD_STRUCT *fpu_owner)
{
}

#endif /* CONFIG_RTL_FP_SUPPORT */

