/* new intercept code */
#include <linux/kernel.h>
#include <linux/module.h>
#include <rtl_core.h>
#include <rtl_sync.h>
#include <asm/pgtable.h>
#include <asm/ptrace.h>

#define NR_EXCEPT 0x30
extern long *intercept_table[NR_EXCEPT];
extern long ret_from_intercept;

long old_except_info[NR_EXCEPT][2];

typedef void (*rtl_trap_handler_t)(struct pt_regs *, int, int);

static int (*rtl_intercepter)(int vector, struct pt_regs *regs, int error_code) = 0;

static int rtl_ex_intercept(struct pt_regs *regs, int par1, int par2)
{
/* 	rtl_printf("going to %x\n", old_except_info[regs->trap / 0x100][0]); */

/* 	rtl_printf("%x ", regs->trap); */
	if (!rtl_intercepter(regs->trap, regs, par1)) {
		((rtl_trap_handler_t)(old_except_info[regs->trap / 0x100][0]))(regs, par1, par2);
		return 1;
	} else  {
		return 0;
	}
}

int setup_intercept(void)
{
	int i;
	rtl_irqstate_t flags;
	rtl_no_interrupts(flags);

/* 	rtl_printf("ret_from_intercept = %x", (&ret_from_intercept)); */
	for (i = 0; i < NR_EXCEPT; i++)  {
		if (!intercept_table[i]) {
			continue;
		}
/* 		rtl_printf("%x: %x %x %x\n", i, (unsigned )(intercept_table[i]), intercept_table[i][0], intercept_table[i][1]); */
		old_except_info[i][0] = intercept_table[i][0];
		old_except_info[i][1] = intercept_table[i][1];
		intercept_table[i][0] = (long) &rtl_ex_intercept;
		intercept_table[i][1] =  (long)&ret_from_intercept;
		flush_icache_range((unsigned) intercept_table[i],(unsigned) intercept_table[i] + 8);
	}
	rtl_restore_interrupts(flags);
	return 0;
}


void restore_intercept(void)
{
	int i;
	rtl_irqstate_t flags;
	rtl_no_interrupts(flags);

	for (i = 0; i < NR_EXCEPT; i++)  {
		if (!intercept_table[i]) {
			continue;
		}
		intercept_table[i][0] = old_except_info[i][0];
		intercept_table[i][1] = old_except_info[i][1];
		flush_icache_range((unsigned) intercept_table[i],(unsigned) intercept_table[i] + 8);
	}
	rtl_restore_interrupts(flags);
}

int rtl_request_traps(int (*rtl_exception_intercept)(int vector, struct pt_regs *regs, int error_code))
{
	if (rtl_exception_intercept) {
		if (rtl_intercepter) {
			return -1;
		} else {
			rtl_intercepter = rtl_exception_intercept;
			setup_intercept();
			return 0;
		}
	} else {
		if (!rtl_intercepter) {
			return -1;
		} else {
			rtl_intercepter = rtl_exception_intercept;
			restore_intercept();
			return 0;
		}
	}
}

