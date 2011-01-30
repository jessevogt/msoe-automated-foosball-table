#include <asm/processor.h>
#include <rtl_conf.h>
	
#define	r0	0
#define	r1	1
#define	r2	2
#define	r3	3
#define	r4	4
#define	r5	5
#define	r6	6
#define	r7	7
#define	r8	8
#define	r9	9
#define	r10	10
#define	r11	11
#define	r12	12
#define	r13	13
#define	r14	14
#define	r15	15
#define	r16	16
#define	r17	17
#define	r18	18
#define	r19	19
#define	r20	20
#define	r21	21
#define	r22	22
#define	r23	23
#define	r24	24
#define	r25	25
#define	r26	26
#define	r27	27
#define	r28	28
#define	r29	29
#define	r30	30
#define	r31	31
	
	.globl __rtl_init_state
__rtl_init_state:
	lwz	r8,0(r3)	/* get current->stack */
	mr	r7,r8
	subi	r8,r8,4*32
	stmw	r0,0(r8)	/* gprs */
	stw	r7,4(r8)	/* stack - r1 */
	stw	r4,12(r8)	/* fn - r3 */
	stw	r5,16(r8)	/* data - r4 */
	stwu	r6,-4(r8)	/* lr */
	mfctr	r7
	stwu	r7,-4(r8)	/* ctr */
	mfcr	r7
	stwu	r7,-4(r8)	/* cr */
	mfspr	r7,XER
	stwu	r7,-4(r8)	/* xer */
	lis 	r7,MSR_KERNEL@h
	ori 	r7,r7,MSR_KERNEL | MSR_FP
	stwu	r7,-4(r8)	/* msr */
	stw	r8,0(r3)	/* store current->stack */
	blr

	.globl __rtl_switch_to
__rtl_switch_to:
	/* hard disable */
	mfmsr	r0
	rlwinm	r0,r0,0,17,15	/* clear MSR_EE in r0 */
	sync
	mtmsr	r0

	/* save old */
	mr	r6,r1
	subi	r6,r6,4*32
	stmw	r0,0(r6)	/* gprs */
	mflr	r5
	stwu	r5,-4(r6)	/* lr */
	mfctr	r5
	stwu	r5,-4(r6)	/* ctr */
	mfcr	r5
	stwu	r5,-4(r6)	/* cr */
	mfxer	r5
	stwu	r5,-4(r6)	/* xer */
	mfmsr	r5
	stwu	r5,-4(r6)	/* msr */
	lwz	r1,0(r3)	/* store current->stack */
	stw	r6,0(r1)

	/* change 'current' */
	stw	r4,0(r3)

	/* restore new */
	
	/* load special regs */
	lwz	r4,0(r4)	/* get new->stack */
	subi	r4,r4,4		/* predecrement */
	lwzu	r1,4(r4)
	mtspr	SRR1,r1		/* msr */
	lwzu	r1,4(r4)
	mtxer	r1		/* xer */
	lwzu	r1,4(r4)
	mtcr	r1		/* cr */
	lwzu	r1,4(r4)
	mtctr	r1		/* ctr */
	lwzu	r1,4(r4)
	mtspr	SRR0,r1		/* lr */

	/* load gprs */
	addi	r4,r4,4		/* preincrement */
	lmw	r5,4*5(r4)	/* gprs */
	lwz	r0,0(r4)
	lwz	r1,4(r4)
	/* skip r2, it is the 'current' pointer and doesn't change -- Cort */
	lwz	r3,12(r4)
	lwz	r4,16(r4)

	sync
	isync
	rfi

#ifdef CONFIG_RTL_FP_SUPPORT

#define RTL_FPREGS_OFFSET 8

#define SAVE_FPR(n, base)	stfd	n,RTL_FPREGS_OFFSET + 8*(n)(base)
#define SAVE_2FPRS(n, base)	SAVE_FPR(n, base); SAVE_FPR(n+1, base)
#define SAVE_4FPRS(n, base)	SAVE_2FPRS(n, base); SAVE_2FPRS(n+2, base)
#define SAVE_8FPRS(n, base)	SAVE_4FPRS(n, base); SAVE_4FPRS(n+4, base)
#define SAVE_16FPRS(n, base)	SAVE_8FPRS(n, base); SAVE_8FPRS(n+8, base)
#define SAVE_32FPRS(n, base)	SAVE_16FPRS(n, base); SAVE_16FPRS(n+16, base)
#define REST_FPR(n, base)	lfd	n,RTL_FPREGS_OFFSET + 8*(n)(base)
#define REST_2FPRS(n, base)	REST_FPR(n, base); REST_FPR(n+1, base)
#define REST_4FPRS(n, base)	REST_2FPRS(n, base); REST_2FPRS(n+2, base)
#define REST_8FPRS(n, base)	REST_4FPRS(n, base); REST_4FPRS(n+4, base)
#define REST_16FPRS(n, base)	REST_8FPRS(n, base); REST_8FPRS(n+8, base)
#define REST_32FPRS(n, base)	REST_16FPRS(n, base); REST_16FPRS(n+16, base)

	.globl __rtl_fpu_save
__rtl_fpu_save:
	mfmsr	r0
	mr	r6,r0
	ori	r0,r0,MSR_FP
	sync
	mtmsr	r0			/* enable use of fpu now */
	isync
	SAVE_32FPRS(0, r3)
	mffs	0
	stfd	0,8*32+8(r3)
	mtmsr	r6
	blr

	.globl __rtl_fpu_restore
__rtl_fpu_restore:
	lwz	r0,4(r3)		/* fpu_initialized */
	cmpi	0,r0,0
	beq	20f
	mfmsr	r0
	mr	r6,r0
	ori	r0,r0,MSR_FP
	sync
	mtmsr	r0			/* enable use of fpu now */
	isync
	lfd	0, 8 * 32 + 8(r3)
	mtfsf	0xff,0
	REST_32FPRS(0, r3)
	mtmsr	r6
	sync
	blr
20:					/* this is our first use of FPU */
	li	r0,1
	stw	r0,4(r3)		/* fpu_initialized */
	blr
#endif
