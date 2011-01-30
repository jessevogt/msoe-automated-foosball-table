#ifndef __RTL_I386_CONSTANTS_H__
#define __RTL_I386_CONSTANTS_H__
#include <linux/config.h>
#define MACHDEPREGS struct pt_regs 
typedef void intercept_t; /*intercept returns nothing */
#define MACHDEPREGS_PTR(x) (&x)
#define POS_TO_BIT(x) (1UL << x)
#define ARCH_DEFINED_DISABLE 0
#define ARCH_DEFINED_ENABLE 0x200

#define IRQ_MAX_COUNT 256
#ifdef CONFIG_X86_IO_APIC
#define __LOCAL_IRQS__
#endif

#include <linux/smp.h>

#ifdef CONFIG_SMP
#define rtl_getcpuid() hw_smp_processor_id()
#else
#define rtl_getcpuid() smp_processor_id()
#endif


/* TODO  in rtl_intercept and rtl_local intercept there is no
   need to have 3 copies of restore_all. The code should 
   simply goto a single block.

   Also the ret_from _intr should be used in rtl_intercept as well.
   No need to do the stupid return. And this address should be patched
   into the code instead of being indirect jumped 
   */
#define RESTORE_ALL\
	"popl %%ebx;\n\t"	\
	"popl %%ecx;\n\t"	\
	"popl %%edx;\n\t"	\
	"popl %%esi;\n\t"	\
	"popl %%edi;\n\t"	\
	"popl %%ebp;\n\t"	\
	"popl %%eax;\n\t"	\
"1:	popl %%ds;\n\t	"\
"2:	popl %%es;\n\t	"\
	"addl $4,%%esp;\n\t"	\
"3:	iret;\n\t	"	

/*  on an intercept, the linux return goes via ret_from_irq
    but the RTLinux return just irets 
*/

#define RETURN_FROM_INTERRUPT_LINUX return
#define RETURN_FROM_INTERRUPT \
        { int i =  (int)&regs;\
	__asm__ __volatile__("movl %0,%%esp\n\t" \
			RESTORE_ALL : /* no output */ :"c" (i):"memory"); }

#define RETURN_FROM_LOCAL RETURN_FROM_INTERRUPT 
#define RETURN_FROM_LOCAL_LINUX \
        { int i =  (int)&regs;\
	__asm__ __volatile__("movl %0,%%esp\n\t"\
		         "jmp *local_ret_from_intr" \
			: /* no output */ :"c" (i):"memory"); }

	/* the PND numbers are between 0 and 31 and are used 
	   in a bitmap of local interrupts */
#define VECTOR_TO_LOCAL_PND_GET(r) VECTOR_TO_LOCAL_PND(r.orig_eax)
#define VECTOR_TO_LOCAL_PND(x)  (((x) - 0xe8))
#define MACHDEPREGS_TO_PND(x)  VECTOR_TO_LOCAL_PND(x.orig_eax)
#define LOCAL_PND_TO_VECTOR(x)  (((x) + 0xe8))

#define RTL_RESCHEDULE_VECTOR 0xEE

/* the intercept function returns 1 if the trap is intercepted;
 * 0 means need to go through Linux code */
extern int rtl_request_traps(int (*rtl_exception_intercept)(int vector, struct pt_regs *regs, int error_code));

#endif  /* __RTL_I386_CONSTANTS_H__ */
