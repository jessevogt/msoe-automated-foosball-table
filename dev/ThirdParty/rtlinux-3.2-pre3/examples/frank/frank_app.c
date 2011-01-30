#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h> /* exit */
#include <sys/ioctl.h>
#include <rtl_fifo.h>
#include <rtl_time.h>
#include "control.h"


#define BUFSIZE 70

char buf[BUFSIZE];

int main()
{
	fd_set rfds;
        struct timeval tv;
        int retval;
	int fd0, fd1, ctl;
	int n;
	int i;
	struct my_msg_struct msg;
	
	if ((fd0 = open("/dev/rtf1", O_RDONLY)) < 0) {
		fprintf(stderr, "Error opening /dev/rtf1\n");
		exit(1);
	}

	if ((fd1 = open("/dev/rtf2", O_RDONLY)) < 0) {
		fprintf(stderr, "Error opening /dev/rtf2\n");
		exit(1);
	}

	if ((ctl = open("/dev/rtf3", O_WRONLY)) < 0) {
		fprintf(stderr, "Error opening /dev/rtf3\n");
		exit(1);
	}

	/* now start the tasks */
	msg.command = START_TASK;
	msg.task = 0;
	msg.period = 500000;
	if (write(ctl, &msg, sizeof(msg)) < 0) {
		fprintf(stderr, "Can't send a command to RT-task\n");
		exit(1);
	}
	msg.task = 1;
	msg.period = 200000;
	if (write(ctl, &msg, sizeof(msg)) < 0) {
		fprintf(stderr, "Can't send a command to RT-task\n");
		exit(1);
	}

	for (i = 0; i < 100; i++) {
		FD_ZERO(&rfds);
		FD_SET(fd0, &rfds);
		FD_SET(fd1, &rfds);
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		retval = select(FD_SETSIZE, &rfds, NULL, NULL, &tv);
		if (retval > 0) {
			if (FD_ISSET(fd0, &rfds)) {
				n = read(fd0, buf, BUFSIZE - 1);
				buf[n] = 0;
				printf("FIFO 1: %s\n", buf);
			}
			if (FD_ISSET(fd1, &rfds)) {
				n = read(fd1, buf, BUFSIZE - 1);
				buf[n] = 0;
				printf("FIFO 2: %s\n", buf);
			}
		}


	}

	fprintf(stderr, "frank_app: now sending commands to stop RT-tasks\n");
	/* stop the tasks */
	msg.command = STOP_TASK;
	msg.task = 0;
	if (write(ctl, &msg, sizeof(msg)) < 0) {
		fprintf(stderr, "Can't send a command to RT-task\n");
		exit(1);
	}
	msg.task = 1;
	if (write(ctl, &msg, sizeof(msg)) < 0) {
		fprintf(stderr, "Can't send a command to RT-task\n");
		exit(1);
	}
	return 0;
}
