#include <asm/asm.h>
#include <asm/bootinfo.h>
#include <asm/cachectl.h>
#include <asm/current.h>
#include <asm/fpregdef.h>
#include <asm/mipsregs.h>
#include <asm/offset.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/processor.h>
#include <asm/regdef.h>
#include <asm/stackframe.h>
#include <asm/asmmacro.h>

/*
 * void __rtl_init_state( task,fn,data,rt_startup )
 */
	.set	noreorder
	.set	noat
	.align	5
	LEAF(__rtl_init_state)
	.set	mips3

	/* a4 <- task->stack-36*4 */
	lw	t0, 0(a0)
	subu	t0, t0, (36*4)

	/* don't play smart, store all regs */
	sw	$1, 1*4(t0)
	sw	$2, 2*4(t0)
	sw	$3, 3*4(t0)
	sw	$4, 4*4(t0)
	sw	$5, 5*4(t0)
	sw	$6, 6*4(t0)
	sw	$7, 7*4(t0)
	sw	$8, 8*4(t0)
	sw	$9, 9*4(t0)
	sw	$10, 10*4(t0)
	sw	$11, 11*4(t0)
	sw	$12, 12*4(t0)
	sw	$13, 13*4(t0)
	sw	$14, 14*4(t0)
	sw	$15, 15*4(t0)
	sw	$16, 16*4(t0)
	sw	$17, 17*4(t0)
	sw	$18, 18*4(t0)
	sw	$19, 19*4(t0)
	sw	$20, 20*4(t0)
	sw	$21, 21*4(t0)
	sw	$22, 22*4(t0)
	sw	$23, 23*4(t0)
	sw	$24, 24*4(t0)
	sw	$25, 25*4(t0)
	sw	$26, 26*4(t0)
	sw	$27, 27*4(t0)
	sw	$28, 28*4(t0)
	sw	$29, 29*4(t0)
	sw	$30, 30*4(t0)
	sw	$31, 31*4(t0)

	/* hi and lo */
	mfhi	t1
	sw	t1, 32*4(t0)
	mflo	t1
	sw	t1, 33*4(t0)
	
	/* ra <- rt_startup */
	sw	a3, 31*4(t0)
	/* arg1 <- fn, arg2 <- data */
	sw	a1, 4*4(t0)
	sw	a2, 5*4(t0)
	/* new stack */
	lw	t1, 0(a0)
	sw	t1, 29*4(t0)

	/* store stack position of saved regs */
	sw	t0, 0(a0)
	
	jr	ra
	nop
	
	END(__rtl_init_state)
	
/*
 * void rtl_switch_to( RTL_THREAD_STRUCT **, RTL_THREAD_STRUCT * )
 */
	.set	noreorder
	.set	noat
	.align	5
	LEAF(rtl_switch_to)
	.set	mips3

	/* a3 <- *current */
	lw	a3, 0(a0)

	/* decrement stack to store regs */
	subu	a2, sp, 36*4

	/* save new stack in current->stack */
	sw	a2, 0(a3)
		
	/* don't play smart, store all regs */
	sw	$1, 1*4(a2)
	sw	$2, 2*4(a2)
	sw	$3, 3*4(a2)
	sw	$4, 4*4(a2)
	sw	$5, 5*4(a2)
	sw	$6, 6*4(a2)
	sw	$7, 7*4(a2)
	sw	$8, 8*4(a2)
	sw	$9, 9*4(a2)
	sw	$10, 10*4(a2)
	sw	$11, 11*4(a2)
	sw	$12, 12*4(a2)
	sw	$13, 13*4(a2)
	sw	$14, 14*4(a2)
	sw	$15, 15*4(a2)
	sw	$16, 16*4(a2)
	sw	$17, 17*4(a2)
	sw	$18, 18*4(a2)
	sw	$19, 19*4(a2)
	sw	$20, 20*4(a2)
	sw	$21, 21*4(a2)
	sw	$22, 22*4(a2)
	sw	$23, 23*4(a2)
	sw	$24, 24*4(a2)
	sw	$25, 25*4(a2)
	sw	$26, 26*4(a2)
	sw	$27, 27*4(a2)
	sw	$28, 28*4(a2)
	sw	$29, 29*4(a2)
	sw	$30, 30*4(a2)
	sw	$31, 31*4(a2)

	/* hi and lo */
	mfhi	t1
	sw	t1, 32*4(a2)
	mflo	t1
	sw	t1, 33*4(a2)
	
	/* *current = next */
	sw	a1, 0(a0)

	/* a1 <- new->stack */
	lw	a2, 0(a1)

	/* restore regs */
	
	/* hi and lo */
	lw	t1, 32*4(a2)
	mthi	t1
	lw	t1, 33*4(a2)
	mtlo	t1
	
	/* general regs */
	lw	$1, 1*4(a2)
	lw	$2, 2*4(a2)
	lw	$3, 3*4(a2)
	/* need to restore a0, a1 so rt_startup() works */
	lw	$4, 4*4(a2)
	lw	$5, 5*4(a2)
	/* skip a2, we're using it now */
	lw	$7, 7*4(a2)
	lw	$8, 8*4(a2)
	lw	$9, 9*4(a2)
	lw	$10, 10*4(a2)
	lw	$11, 11*4(a2)
	lw	$12, 12*4(a2)
	lw	$13, 13*4(a2)
	lw	$14, 14*4(a2)
	lw	$15, 15*4(a2)
	lw	$16, 16*4(a2)
	lw	$17, 17*4(a2)
	lw	$18, 18*4(a2)
	lw	$19, 19*4(a2)
	lw	$20, 20*4(a2)
	lw	$21, 21*4(a2)
	lw	$22, 22*4(a2)
	lw	$23, 23*4(a2)
	lw	$24, 24*4(a2)
	lw	$25, 25*4(a2)
	lw	$26, 26*4(a2)
	lw	$27, 27*4(a2)
	lw	$28, 28*4(a2)
	lw	$29, 29*4(a2)
	lw	$30, 30*4(a2)
	lw	$31, 31*4(a2)

	/* load a2 now that we're done with it */
	lw	$6, 6*4(a2)

	jr	$31
	nop
	END(rtl_switch_to)
