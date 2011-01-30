
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <rtl_fifo.h>

int 
main(int argc,char ** argv)
{
	int fd0;
	int dc=10;

	if ((fd0 = open("/dev/rtf0", O_WRONLY)) < 0) {
		fprintf(stderr, "Error opening /dev/rtf0\n");
		exit(1);
	}

	while(dc!=0){
		printf("Duty Cycle [10-90%%]: ");
		scanf("%d",&dc);
		write(fd0, &dc, sizeof(dc));
		printf("Duty cycle set to %d %%\n", dc);
		fflush(stdout);
	}
	return 0;
}
