/*
 * (C) Finite State Machine Labs Inc. 2000 business@fsmlabs.com
 *
 * Released under the terms of GPL 2.
 * Open RTLinux makes use of a patented process described in
 * US Patent 5,995,745. Use of this process is governed
 * by the Open RTLinux Patent License which can be obtained from
 * www.fsmlabs.com/PATENT or by sending email to
 * licensequestions@fsmlabs.com
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

int main(void)
{
	int fd;
	char inbuf[15];
	int i;

	fd = open("/dev/rtf0", O_RDONLY);

	for (i = 0; i < 150; i++) {
		read(fd, inbuf, 14);
		if ((strncmp("I'm still here", inbuf, 15)) != 0) {
			fprintf(stderr, "inbuf: %s\n", inbuf);
			return -1;
		}
	}

	return 0;
}
