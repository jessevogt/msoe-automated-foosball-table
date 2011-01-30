/*
 * Copywrite 2002 Der Herr Hofrat
 * License GPL V2
 * Author der.herr@hofr.at
 */
/*
 * prototype usage of loadable system call - this will grab the dynamically
 * allocated system cal number from /proc/pal_sys_nr and then trigger the
 * system call via syscall(#) - run demesg to check output from the actual
 * system call.
 */ 
#include <stdio.h>      /* printf */
#include <asm/unistd.h> /* __NR_test_call _syscall0() */
#include <errno.h>      /* for syscall() _syscall0() */

int main(int argc,char ** argv){
	int syscall_nr;
	FILE *pal_sys_nr;

	/* grab the syscall number to use */
	if((pal_sys_nr = fopen("/proc/pal_sys_nr","r")) == NULL )
	{
		perror("Cannot open /proc/pal_sys_nr - did you load the modules ?");
		exit(-1);
	}
	fscanf( pal_sys_nr, "PAL syscall Nr:%d\n", &syscall_nr );
	fclose(pal_sys_nr);

	/* invoce the system call now */	
	syscall(syscall_nr);
	printf("now type  dmesg  to see the printk from default pal syscall\n");
	return 0;
}
