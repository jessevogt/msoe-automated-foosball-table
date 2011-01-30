#include <linux/version.h>
#define __STR(x) #x
#define STR(x) __STR(x)

extern void rtl_fpu_save (schedule_t *s, RTL_THREAD_STRUCT *task);
extern void rtl_fpu_restore (schedule_t *s,RTL_THREAD_STRUCT *task);
extern void rtl_task_init_fpu (RTL_THREAD_STRUCT *t, RTL_THREAD_STRUCT *fpu_owner);

#define rtl_switch_to(current_task_ptr, new_task) \
	__asm__ __volatile__( \
	"pushl %%eax\n\t" \
	"pushl %%ebp\n\t" \
	"pushl %%edi\n\t" \
	"pushl %%esi\n\t" \
	"pushl %%edx\n\t" \
	"pushl %%ecx\n\t" \
	"pushl %%ebx\n\t" \
	"movl (%%ebx), %%edx\n\t" /* get current */ \
	"pushl $1f\n\t" \
	"movl %%esp, (%%edx)\n\t" \
	"movl (%%ecx), %%esp\n\t" \
	"movl %%ecx, (%%ebx)\n\t" /* store current */\
	"ret\n\t" \
"1:	popl %%ebx\n\t" \
	"popl %%ecx\n\t" \
	"popl %%edx\n\t" \
	"popl %%esi\n\t" \
	"popl %%edi\n\t" \
	"popl %%ebp\n\t" \
	"popl %%eax\n\t" \
	: /* no output */ \
	: "c" (new_task), "b" (current_task_ptr) \
	);

#define rtl_init_stack(task,fn,data,rt_startup)\
	{\
	*--(task->stack) = (int) data;\
	*--(task->stack) = (int) fn;\
	*--(task->stack) = 0;	/* dummy return addr*/\
	*--(task->stack) = (int) rt_startup;\
	}

typedef struct {
	unsigned long cr3;
} rtl_mmu_state_t;

#define rtl_mmu_save(mmu_state) __asm__ __volatile__("movl %%cr3,%0":"=r" (*(mmu_state)) :)

#if LINUX_VERSION_CODE >= 0x020300
#define rtl_mmu_switch_to(tsk) do { __asm__ __volatile__("movl %0,%%cr3\n\tmovl %0,%%cr3": :"r" (__pa(((tsk)->mm)->pgd))); } while (0)
#else
#define rtl_mmu_switch_to(tsk) do { __asm__ __volatile__("movl %0,%%cr3\n\tmovl %0,%%cr3": :"r" ((tsk)->tss.cr3)); } while (0)
#endif

#define rtl_mmu_restore(mmu_state) __asm__ __volatile__("movl %0,%%cr3": :"r" (*(mmu_state)))

#define RTL_PSC_NEW 1

