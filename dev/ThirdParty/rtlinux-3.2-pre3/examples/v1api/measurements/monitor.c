#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <rtl_fifo.h>
#include <asm/rt_time.h>
#include "common.h"



int main()
{
	int fd0;
	int n;
	struct sample samp;
	
	if ((fd0 = open("/dev/rtf0", O_RDONLY)) < 0) {
		fprintf(stderr, "Error opening /dev/rtf0\n");
		exit(1);
	}


	while (1) {
		n = read(fd0, &samp, sizeof(samp));
		printf("min: %8d, max: %8d\n", (int) samp.min, (int) samp.max);
		fflush(stdout);
	}

	return 0;
}
