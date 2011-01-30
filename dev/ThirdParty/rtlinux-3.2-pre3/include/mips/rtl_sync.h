/*
 * (C) Finite State Machine Labs Inc. 2000 business@fsmlabs.com
 *
 * Released under the terms of GPL 2.
 * Open RTLinux makes use of a patented process described in
 * US Patent 5,995,745. Use of this process is governed
 * by the Open RTLinux Patent License which can be obtained from
 * www.fsmlabs.com/PATENT or by sending email to
 * licensequestions@fsmlabs.com
 */

#ifndef __ARCH_RTL_SYNC__
#define __ARCH_RTL_SYNC__

#include <linux/irq.h>
#include <asm/system.h>
#include <asm/hw_irq.h>

typedef unsigned long rtl_irqstate_t;

extern void (*mips_hard_sti) (void);
extern void (*mips_hard_cli) (void);
extern void (*mips_hard_save_flags_ptr) (unsigned long *);
extern void (*mips_hard_save_and_cli_ptr) (unsigned long *);
extern void (*mips_hard_restore_flags) (ulong);

#define __rtl_hard_cli() ({			\
	if ( mips_hard_cli ) mips_hard_cli();	\
	else panic("No mips_hard_cli()\n");	\
})

#define __rtl_hard_sti() ({			\
	if ( mips_hard_sti ) mips_hard_sti();	\
	else panic("No mips_hard_sti()\n");	\
})

#define __rtl_hard_save_flags(x) ({					\
	if ( mips_hard_save_flags_ptr ) mips_hard_save_flags_ptr(&(x));	\
	else panic("No mips_hard_save_flags()\n");			\
})

#define __rtl_hard_restore_flags(x) ({				  \
	if ( mips_hard_restore_flags ) mips_hard_restore_flags(x);\
	else panic("No mips_hard_restore_flags()\n");		  \
})

#define __rtl_hard_savef_and_cli(x) ({					\
	if ( mips_hard_save_and_cli_ptr ) mips_hard_save_and_cli_ptr(&(x));\
	else panic("No mips_hard_save_and_cli_ptr()\n");		\
})

#endif				/* __ARCH_RTL_SYNC__ */
