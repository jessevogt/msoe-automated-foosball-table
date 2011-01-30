/* see README.syscall for what this is about....*/
#include <asm/unistd.h> /* __NR_test_call _syscall0() */
#include <errno.h>      /* for syscall() _syscall0() */

_syscall0(int,test_call); 

main(){
	syscall(222); /* call it via syscal(SYSCAL_NUMBER) */
	test_call();  /* call it by name */
	return 0;
}
