#include <sys/ioctl.h> /* ioctl */
#include <sys/types.h> /* open */
#include <sys/stat.h>  /* open */
#include <fcntl.h>     /* open */
#include <unistd.h>    /* close */
#include <stdio.h>     /* fprintf */
#include <stdlib.h>    /* exit */
#include "my_ioctl.h"  /* ioctl cmd numbers */

int main(int argc, char ** argv){
	int fd;
	
	if ((fd = open("/dev/rtf0", O_RDWR)) < 0) {
		fprintf(stderr, "Error opening /dev/rtf0\n");
		exit(1);
	}

	ioctl(fd, SHOW_SIZE);
	ioctl(fd, FLUSH);
	close(fd);
	return 0;
}

