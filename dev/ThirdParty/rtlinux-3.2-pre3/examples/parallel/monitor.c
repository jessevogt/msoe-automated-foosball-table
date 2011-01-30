/*
 * RTL scheduling accuracy measuring example, user program
 *
 * (C) Michael Barabanov, 1997
 *  (C) FSMLabs  1999. baraban@fsmlabs.com
 *  Released under the GNU GENERAL PUBLIC LICENSE Version 2, June 1991
 *  Any use of this code must include this notice.
 */

#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <rtl_fifo.h>
#include "common.h"



int 
main()
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

	exit(0);
}
