#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <rtl_fifo.h>
#include "common.h"



int main()
{
	int fd0;
	int n;
	int samp;
	
	if ((fd0 = open("/dev/rtf0", O_RDONLY)) < 0) {
		fprintf(stderr, "Error opening /dev/rtf0 %dn",fd0);
		perror("perror");
		exit(1);
	}


	while (1) {
		n = read(fd0, &samp, sizeof(samp));
		printf("%d\n", (int) samp);
		fflush(stdout);
	}

	return 0;
}
