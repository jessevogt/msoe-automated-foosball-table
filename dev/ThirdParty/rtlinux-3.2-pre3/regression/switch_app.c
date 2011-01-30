/*
 * (C) Finite State Machine Labs Inc. 2001 business@fsmlabs.com
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

#include "switch_test.h"

#ifndef	hrtime_t
typedef long long int hrtime_t;
#endif				/* hrtime_t */

/* FIFO number to use */
#define	FIFO_NR	1
/* FIFO size to use */
#define	FIFO_SZ	16384

/* module unload and loading commands */
#define	MODULE_LOAD	"/sbin/insmod regression/switch_time.o fifo_nr=%d fifo_sz=%d"
#define MODULE_UNLOAD	"/sbin/rmmod switch_time"

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
			(int) sizeof(struct sigaction), strerror(errno));
		return errno;
	}

	my_action->sa_handler = &my_sahandler;

	if (
	    (old_action =
	     (struct sigaction *) calloc(1,
					 sizeof(struct sigaction))) ==
	    NULL) {
		fprintf(stderr, "calloc(1, %d): %s\n",
			(int) sizeof(struct sigaction), strerror(errno));
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
	int retval, filedes, i, copy_size;
	hrtime_t time1[num_tests], time2[num_tests], worst, best, current;
	double average;
	char *filename, *tmp_buf;
	struct pollfd fifo_poll;

	average = 0;
	worst = 0;
	best = 99999999;
	copy_size = (sizeof(hrtime_t) * (num_tests));

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
		return errno;
	}

	fifo_poll.fd = filedes;
	fifo_poll.events = POLLIN;

	if ((poll(&fifo_poll, 1, 1000000 * num_tests)) < 0) {
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
		memcpy(&time1, tmp_buf, copy_size);
		memcpy(&time2, tmp_buf + copy_size, copy_size);
	}

	for (i = 0; i < num_tests; i++) {
		current = time2[i] - time1[i];
		average += current;
		worst = (worst < current) ? current : worst;
		best = (best > current) ? current : best;
	}

	average /= num_tests;

	fprintf(stderr,
		"average: %f us\tworst: %d.%02d us\tbest: %d.%02d us\n",
		average / 1000, (int) worst / 1000,
		(int) (worst % 1000) / 10, (int) best / 1000,
		(int) (best % 1000) / 10);

	close(filedes);
	free(filename);
	unload_module();
	return 0;
}
