	.align 3
	.set	noat
	.ent	__rtl_init_state
__rtl_init_state:
	ldq	$9,0($16)	/* get current->stack */
	lda	$7,0($9)	/* save old stack */
	
	lda	$9,(-8*32)($9)	/* decrement for 32 regs */

	/* save copies of the current regs */
	stq	$0,8*0($9)
	stq	$1,8*1($9)
	stq	$2,8*2($9)
	stq	$3,8*3($9)
	stq	$4,8*4($9)
	stq	$5,8*5($9)
	stq	$6,8*6($9)
	stq	$7,8*7($9)
	stq	$8,8*8($9)
	stq	$9,8*9($9)
	stq	$10,8*10($9)
	stq	$11,8*11($9)
	stq	$12,8*12($9)
	stq	$13,8*13($9)
	stq	$14,8*14($9)
	stq	$15,8*15($9)
	stq	$16,8*16($9)
	stq	$17,8*17($9)
	stq	$18,8*18($9)
	stq	$19,8*19($9)
	stq	$20,8*20($9)
	stq	$21,8*21($9)
	stq	$22,8*22($9)
	stq	$23,8*23($9)
	stq	$24,8*24($9)
	stq	$25,8*25($9)
	stq	$26,8*26($9)
	stq	$27,8*27($9)
	stq	$28,8*28($9)
	stq	$29,8*29($9)
	stq	$30,8*30($9)
	
	/* now save the regs we need to change */
	stq	$7,8*30($9)	/* stack - r30 */
	stq	$17,8*16($9)	/* fn - r16 */
	stq	$18,8*17($9)	/* data - r17 */
	stq	$19,8*26($9)	/* rt_startup - ra */
	stq	$19,8*27($9)	/* rt_startup - function pointer */
	stq	$9,0($16)	/* store stack */

	ret
	.end __rtl_init_state

	.align 3
	.set	noat
	.ent	__restore
__restore:
	stq	$17,0($16)	/* update current - store *new in **current */
	ldq	$19,0($17)	/* get stack */

	/* restore non-volatiles */
	ldq	$9,(8*9)($19)
	ldq	$10,(8*10)($19)
	ldq	$11,(8*11)($19)
	ldq	$12,(8*12)($19)
	ldq	$13,(8*13)($19)
	ldq	$14,(8*14)($19)
	/* fp */
	ldq	$15,(8*15)($19)
	/* restore arg regs - they're volatile but on startup we need
	 * the first two --Cort 
	 */
	ldq	$16,(8*16)($19)
	ldq	$17,(8*17)($19)
	/* restore ra */
	ldq	$26,(8*26)($19)
	ldq	$27,(8*27)($19)
	/* stack */
	ldq	$30,(8*30)($19)
	
	lda	$0,0($31)
	
	ret
	.end __restore

	.align 3
	.set	noat
	.ent	__save
__save:
	lda	$19,-8*32($30)	/* decrement stack */
	stq	$19,0($16)	/* store new stack */
	
	/* save copies of the current regs */
	stq	$9,8*9($19)
	stq	$10,8*10($19)
	stq	$11,8*11($19)
	stq	$12,8*12($19)
	stq	$13,8*13($19)
	stq	$14,8*14($19)
	stq	$15,8*15($19)
	stq	$26,8*26($19)
	stq	$27,8*27($19)
	stq	$30,8*30($19)
	
	lda	$0,1($31)
	
	ret
	.end __save
