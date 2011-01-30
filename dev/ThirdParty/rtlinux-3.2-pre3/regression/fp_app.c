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
#include <sys/poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

/* FIFO number to use */
#define	FIFO_NR	1
/* FIFO size to use */
#define	FIFO_SZ	8192

/* module unload and loading commands */
#define	MODULE_LOAD	"/sbin/insmod regression/fp_test.o fifo_nr=%d fifo_sz=%d"
#define MODULE_UNLOAD	"/sbin/rmmod fp_test"

int unload_module()
{
	if ((system(MODULE_UNLOAD)) != 0) {
		fprintf(stderr, "system(%s): %s\n", MODULE_UNLOAD,
			strerror(errno));
		return errno;
	}

	return 0;
}

void my_sahandler(int whatever)
{
	fprintf(stderr, "my_sahandler: received signal %d\n", whatever);
	unload_module();
	exit(whatever);
}

int sig_handler_setup(void)
{
	struct sigaction *my_action;
	struct sigaction *old_action;

	if (
	    (my_action =
	     (struct sigaction *) calloc(1,
					 sizeof(struct sigaction))) ==
	    NULL) {
		fprintf(stderr, "calloc(1, %d): %s\n",
			sizeof(struct sigaction), strerror(errno));
		return errno;
	}

	my_action->sa_handler = &my_sahandler;

	if (
	    (old_action =
	     (struct sigaction *) calloc(1,
					 sizeof(struct sigaction))) ==
	    NULL) {
		fprintf(stderr, "calloc(1, %d): %s\n",
			sizeof(struct sigaction), strerror(errno));
		return errno;
	}

	sigaction(SIGHUP, my_action, old_action);
	sigaction(SIGINT, my_action, old_action);
	sigaction(SIGQUIT, my_action, old_action);

	return 0;
}

int load_module(int fifo_nr, int fifo_sz)
{
	int i = strlen(MODULE_LOAD) + 32;
	char command[i];

	if ((snprintf(command, i, MODULE_LOAD, fifo_nr, fifo_sz)) < 0) {
		fprintf(stderr, "snprintf(): %s", strerror(errno));
		return errno;
	}

	if ((system(command)) != 0) {
		fprintf(stderr, "system(%s): %s\n", command,
			strerror(errno));
		return errno;
	}

	return 0;
}

char *construct_filename(int i)
{
	char *filename;

	if ((filename = (char *) calloc(11, sizeof(char))) == NULL) {
		fprintf(stderr, "calloc(11, sizeof(char)): %s\n",
			strerror(errno));
		return NULL;
	}

	if ((snprintf(filename, 11, "/dev/rtf%d", i)) < 0) {
		fprintf(stderr, "snprintf(filename, 11, /dev/rtf%d): %s",
			i, strerror(errno));
		free(filename);
		return NULL;
	}

	return filename;
}

int stat_test(const char *filename)
{
	struct stat file_stats;

	if (stat(filename, &file_stats) != 0) {
		fprintf(stderr, "stat(%s, &file_stats): %s\n", filename,
			strerror(errno));
		return errno;
	}

	if (!(S_ISCHR(file_stats.st_mode))) {
		fprintf(stderr, "%s is not a character device.\n",
			filename);
		return -1;
	}

	return 0;
}

int main(void)
{
	int retval, filedes;
	float marker1 = 0, marker2 = 0, marker3 = 0, marker4 = 0;
	char *filename, *tmp_buf, *tmp_buf2;
	struct pollfd fifo_poll;

	unload_module();

	sig_handler_setup();

	if ((tmp_buf = (char *) calloc(FIFO_SZ, 1)) == NULL) {
		fprintf(stderr, "calloc(FIFO_SZ, 1): %s\n",
			strerror(errno));
		return errno;
	}

	if ((retval = load_module(FIFO_NR, FIFO_SZ)) < 0) {
		return retval;
	}

	if ((filename = construct_filename(FIFO_NR)) == NULL) {
		unload_module();
		return -1;
	}

	if ((retval = stat_test(filename)) < 0) {
		unload_module();
		free(filename);
		return retval;
	}

	if ((filedes = open(filename, O_RDONLY | O_NONBLOCK)) < 0) {
		fprintf(stderr, "open(%s, O_RDONLY): %s\n", filename,
			strerror(errno));
		unload_module();
		free(filename);
		return errno;
	}
	free(filename);

	fifo_poll.fd = filedes;
	fifo_poll.events = POLLIN;

	while ((marker1 == 0) || (marker2 == 0) || (marker3 == 0)
	       || (marker4 == 0)) {
		if ((poll(&fifo_poll, 1, 1)) < 0) {
			fprintf(stderr, "poll(&fifo_poll, 1, 1): %s\n",
				strerror(errno));
			close(filedes);
			unload_module();
			return errno;
		}

		if ((retval = read(filedes, tmp_buf, FIFO_SZ)) < 0) {
			fprintf(stderr, "read(%d, tmp_buf, FIFO_SZ): %s\n",
				filedes, strerror(errno));
			close(filedes);
			unload_module();
			return errno;
		}

		if (retval > 0) {
			if (tmp_buf[0] == 'h') {
				if ((strncmp(tmp_buf, "handler1", 8)) == 0) {
					tmp_buf2 = strsep(&tmp_buf, ":");
					tmp_buf2 = strsep(&tmp_buf, ":");
					memcpy(&marker1, tmp_buf2,
					       sizeof(marker1));
					tmp_buf2 = strsep(&tmp_buf, ":");
					memcpy(&marker3, tmp_buf2,
					       sizeof(marker1));
				}
				if ((strncmp(tmp_buf, "handler2", 8)) == 0) {
					tmp_buf2 = strsep(&tmp_buf, ":");
					tmp_buf2 = strsep(&tmp_buf, ":");
					memcpy(&marker2, tmp_buf2,
					       sizeof(marker1));
					tmp_buf2 = strsep(&tmp_buf, ":");
					memcpy(&marker4, tmp_buf2,
					       sizeof(marker1));
				}
			}
		}
	}

	/* these values are really ugly-huge.  i may have to actually change
	 * the math so that they come out to something more reasonable. */
	if (marker1 != 3.085398197174072265625) {
		fprintf(stderr, "marker1 is %f\n", marker1);
		return (int) marker1;
	}
	if (marker2 != -.02500014938414096832275390625) {
		fprintf(stderr, "marker2 is %f\n", marker2);
		return (int) marker2;
	}
	if (marker3 != -1.10602283477783203125) {
		fprintf(stderr, "marker3 is %f\n", marker3);
		return (int) marker3;
	}
	if (marker4 != -.2472789585590362548828125) {
		fprintf(stderr, "marker4 is %f\n", marker4);
		return (int) marker4;
	}

	close(filedes);
	unload_module();
	return 0;
}
