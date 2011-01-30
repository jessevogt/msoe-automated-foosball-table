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
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>

#include "fifo_test.h"

/* module unload and loading commands */
#define	MODULE_LOAD	"/sbin/insmod regression/fifo_module.o fifo_size=%d fifo_nr=%d"
#define MODULE_UNLOAD	"/sbin/rmmod fifo_module"

/* fifo size params */
#define	MIN_SIZE	1
#define	MAX_SIZE	65536
/* this is set to 32 to make the test go faster; it will test fifos of size 1,
 * 32, 1024 and 32768 (even though the MAX_SIZE is 65536).  Set this number to
 * 2 or any other multiple of 2 smaller than 32 to make it be more thorough
 * and slow; set it to any multiple higher than 32 to make it go faster, but
 * be less thorough. */
#define	MUL_SIZE	2

char *construct_filename(int);
int stat_test(const char *);
int open_test_fail(const char *);
int load_module();
int over_write_test(const char *, int);
int over_read_test(const char *, int);
char *read_test(const char *, int, int);
int unload_module();
int write_test(const char *, const char *, int, int);
int write_read_test(const char *, int);
char *get_random_str(int);
int sig_handler_setup(void);
void my_sahandler(int);
int big_fifo_test(int);

int main(void)
{
	int i;
	int retval;
	int test_nr, fifo_size, buf_size;
	char *filename, *teststr, *resstr;

	/* set up handler to unload module if we get ^C or similar */
	if ((retval = sig_handler_setup()) != 0) {
		return (retval);
	}

	/* attempt to unload the module because the signal handler doesn't
	 * always manage to do it when we get ^C */
	unload_module();

	/* outer loop to test ALL the FIFOs */
	/* instead of looping through any arbitrary FIFOs, just do the
	 * important ones */

	if ((retval = big_fifo_test(1)) != 0) {
		return retval;
	}

	if ((retval = big_fifo_test(32)) != 0) {
		return retval;
	}

	if ((retval = big_fifo_test(63)) != 0) {
		return retval;
	}

	if ((retval = big_fifo_test(10)) != 0) {
		return retval;
	}

	if ((retval = big_fifo_test(11)) != 0) {
		return retval;
	}

	if ((retval = big_fifo_test(12)) != 0) {
		return retval;
	}

	/* now try testing to see how big a FIFO we can make */
	/* don't do this -Nathan
	   i = 1;
	   retval = 0;
	   while (retval == 0) {
	   retval = load_module(1, i);
	   unload_module();
	   i *= 2;
	   }
	 */
	return (0);
}

char *construct_filename(int i)
{
	char *filename;

	if ((filename = (char *) calloc(11, sizeof(char))) == NULL) {
		fprintf(stderr, "calloc (11, sizeof (char)): %s\n",
			strerror(errno));
		return (NULL);
	}

	if ((snprintf(filename, 11, "/dev/rtf%d", i)) < 0) {
		fprintf(stderr, "snprintf (filename, 11, /dev/rtf%d): %s",
			i, strerror(errno));
		free(filename);
		return (NULL);
	}

	return (filename);
}

int stat_test(const char *filename)
{
	struct stat file_stats;

	if (stat(filename, &file_stats) != 0) {
		fprintf(stderr, "stat (%s, &file_stats): %s\n", filename,
			strerror(errno));
		return (errno);
	}

	if (!(S_ISCHR(file_stats.st_mode))) {
		fprintf(stderr, "%s is not a character device.\n",
			filename);
		return (-1);
	}

	return (0);
}

/* this function assumes the module is not already loaded */
int open_test_fail(const char *filename)
{
	int filedes;

	if ((filedes = open(filename, O_RDONLY)) > 0) {
		fprintf(stderr,
			"Opened %s for read without module loaded!\n",
			filename);
		if ((close(filedes)) != 0) {
			fprintf(stderr, "close (%d): %s\n", filedes,
				strerror(errno));
			return (errno);
		}
		return (-1);
	}

	if ((filedes = open(filename, O_WRONLY | O_NONBLOCK)) > 0) {
		fprintf(stderr,
			"Opened %s for write without module loaded!\n",
			filename);
		if ((close(filedes)) != 0) {
			fprintf(stderr, "close (%d): %s\n", filedes,
				strerror(errno));
			return (errno);
		}
		return (-1);
	}

	if ((filedes = open(filename, O_RDWR)) > 0) {
		fprintf(stderr,
			"Opened %s for read/write without module loaded!\n",
			filename);
		if ((close(filedes)) != 0) {
			fprintf(stderr, "close (%d): %s\n", filedes,
				strerror(errno));
			return (errno);
		}
		return (-1);
	}

	return (0);
}

int load_module(int fifo_size, int fifo_nr)
{
	int i = strlen(MODULE_LOAD) + 4;
	char command[i];

	if ((snprintf(command, i, MODULE_LOAD, fifo_size, fifo_nr)) < 0) {
		fprintf(stderr, "snprintf (): %s", strerror(errno));
		return (errno);
	}

	i = system(command);
	if ((i == 127) || (i == -1)) {
		fprintf(stderr, "system (%s): %d %s\n", command, i,
			strerror(errno));
		return (errno);
	}

	return (0);
}

int unload_module()
{
	int i;

	i = system(MODULE_UNLOAD);
	if ((i == 127) || (i == -1)) {
		fprintf(stderr, "system (%s): %d %s\n", MODULE_UNLOAD, i,
			strerror(errno));
		return (errno);
	}

	return (0);
}

int over_read_test(const char *filename, int size)
{
	int filedes;
	int read_size;
	char *inbuf;

	if ((inbuf = (char *) calloc(size + 1, sizeof(char))) == NULL) {
		fprintf(stderr, "calloc (1, %d): %s\n", sizeof(char),
			strerror(errno));
		return (errno);
	}

	if ((filedes = open(filename, O_RDONLY | O_NONBLOCK)) < 0) {
		fprintf(stderr, "open (%s, O_RDONLY): %s\n", filename,
			strerror(errno));
		free(inbuf);
		return (errno);
	}

	if ((read_size = read(filedes, inbuf, size + 1)) > size) {
		fprintf(stderr, "read %d bytes from %d size FIFO!\n",
			read_size, size);
		close(filedes);
		free(inbuf);
		return (-1);
	}

	free(inbuf);

	if ((close(filedes)) != 0) {
		fprintf(stderr, "close (%d): %s\n", filedes,
			strerror(errno));
		return (errno);
	}

	return (0);
}

char *read_test(const char *filename, int buf_size, int size)
{
	int filedes;
	int read_size;
	int inbuf_size;
	char *inbuf;
	struct pollfd fifo_poll;

	read_size = 1;
	inbuf_size = 0;

	if ((inbuf = (char *) calloc(size, sizeof(char))) == NULL) {
		fprintf(stderr, "calloc (1, %d): %s\n", sizeof(char),
			strerror(errno));
		return (NULL);
	}

	if ((filedes = open(filename, O_RDONLY | O_NONBLOCK)) < 0) {
		fprintf(stderr, "open (%s, O_RDONLY): %s\n", filename,
			strerror(errno));
		free(inbuf);
		return (NULL);
	}

	/* wait for data to arrive in FIFO */
	fifo_poll.fd = filedes;
	fifo_poll.events = POLLIN;
	if ((poll(&fifo_poll, 1, 5000)) <= 0) {
		fprintf(stderr, "poll (&fifo_poll, 1, 5000): %s\n",
			strerror(errno));
		free(inbuf);
		close(filedes);
		return (NULL);
	}
	/* attempt to empty the FIFO buf_size blocks at a time */
	while (inbuf_size < size) {
		if (
		    (read_size =
		     read(filedes, inbuf + inbuf_size, buf_size)) < 0) {
			fprintf(stderr, "read (%d, inbuf, %d): %s\n",
				filedes, buf_size, strerror(errno));
			close(filedes);
			free(inbuf);
			return (NULL);
		}
		inbuf_size += read_size;
	}

	if ((close(filedes)) != 0) {
		fprintf(stderr, "close (%d): %s\n", filedes,
			strerror(errno));
		free(inbuf);
		return (NULL);
	}

	return (inbuf);
}

int over_write_test(const char *filename, int size)
{
	int filedes;
	int write_size;
	char *outbuf;

	if ((outbuf = (char *) calloc(size + 1, sizeof(char))) == NULL) {
		fprintf(stderr, "calloc (%d, %d): %s\n", 2 * size,
			sizeof(char), strerror(errno));
		return (errno);
	}

	if ((filedes = open(filename, O_WRONLY | O_NONBLOCK)) < 0) {
		fprintf(stderr, "open (%s, O_RDONLY): %s\n", filename,
			strerror(errno));
		free(outbuf);
		return (errno);
	}

	if ((write_size = write(filedes, outbuf, size + 1)) > size) {
		fprintf(stderr, "wrote %d bytes to %d size FIFO!\n",
			write_size, size);
		free(outbuf);
		close(filedes);
		return (-1);
	}

	free(outbuf);

	if ((close(filedes)) != 0) {
		fprintf(stderr, "close (%d): %s\n", filedes,
			strerror(errno));
		return (errno);
	}

	return (0);
}

int write_test(const char *filename, const char *outbuf, int buf_size,
	       int size)
{
	int filedes;
	int write_size;
	int sofar;
	struct pollfd fifo_poll;

	write_size = 1;
	sofar = 0;

	if ((filedes = open(filename, O_WRONLY | O_NONBLOCK)) < 0) {
		fprintf(stderr, "open (%s, O_RDONLY): %s\n", filename,
			strerror(errno));
		return (errno);
	}

	/* attempt to fill the FIFO buf_size chunks at a time */
	fifo_poll.fd = filedes;
	fifo_poll.events = POLLOUT;
	while (sofar < size) {
		if ((poll(&fifo_poll, 1, 5000)) <= 0) {
			fprintf(stderr, "poll (&fifo_poll, 1, 5000): %s\n",
				strerror(errno));
			close(filedes);
			return (errno);
		}
		if ((write_size = write(filedes, outbuf + sofar, buf_size))
		    < 0) {
			fprintf(stderr, "write (%d, outbuf, %d): %s\n",
				filedes, buf_size, strerror(errno));
			close(filedes);
			return (errno);
		}
		sofar += write_size;
	}

	if ((close(filedes)) != 0) {
		fprintf(stderr, "close (%d): %s\n", filedes,
			strerror(errno));
		return (errno);
	}

	return (0);
}

int write_read_test(const char *filename, int buf_size)
{
	int filedes;
	char *outbuf;
	char inbuf[buf_size];
	struct pollfd fifo_poll;

	if ((outbuf = get_random_str(buf_size)) == NULL) {
		return (-1);
	}

	if ((filedes = open(filename, O_RDWR | O_NONBLOCK)) < 0) {
		fprintf(stderr, "open (%s, O_RDWR): %s\n", filename,
			strerror(errno));
		free(outbuf);
		return (errno);
	}

	/* write some stuff to the FIFO */
	fifo_poll.fd = filedes;
	fifo_poll.events = POLLOUT;
	if ((poll(&fifo_poll, 1, 5000)) <= 0) {
		fprintf(stderr, "poll (&fifo_poll, 1, 5000): %s\n",
			strerror(errno));
		free(outbuf);
		close(filedes);
		return (errno);
	}
	if ((write(filedes, outbuf, buf_size)) < 0) {
		fprintf(stderr, "write (%d, %s, %d): %s\n", filedes,
			outbuf, buf_size, strerror(errno));
		free(outbuf);
		close(filedes);
		return (errno);
	}

	/* wait for data to arrive in FIFO */
	fifo_poll.events = POLLIN;
	if ((poll(&fifo_poll, 1, 5000)) <= 0) {
		fprintf(stderr, "poll (&fifo_poll, 1, 5000): %s\n",
			strerror(errno));
		free(outbuf);
		close(filedes);
		return (errno);
	}
	if ((read(filedes, inbuf, buf_size)) < 0) {
		fprintf(stderr, "read (%d, inbuf, %d): %s\n", filedes,
			buf_size, strerror(errno));
		free(outbuf);
		close(filedes);
		return (errno);
	}

	if ((close(filedes)) != 0) {
		fprintf(stderr, "close (%d): %s\n", filedes,
			strerror(errno));
		free(outbuf);
		return (errno);
	}

	/* make sure what we read is the same as what was sent */
	if ((memcmp(outbuf, inbuf, buf_size)) != 0) {
		fprintf(stderr, "inbuf is not the same as outbuf\n");
		free(outbuf);
		return (-1);
	}

	free(outbuf);
	return (0);
}

/* get random bytes from /dev/urandom and put them in a string */
/* NOTE: this does NOT return NULL ("\0") terminated strings, so you 
 * cannot use strlen() on strings that this function returns without
 * adding your own terminating NULL or changing this function */
char *get_random_str(int size)
{
	char *randstr;
	int randfile;

	if ((randstr = (char *) calloc(size, sizeof(char))) == NULL) {
		fprintf(stderr, "calloc (%d, %d): %s\n", size,
			sizeof(char), strerror(errno));
		return (NULL);
	}

	if ((randfile = open("/dev/urandom", O_RDONLY | O_NONBLOCK)) < 0) {
		fprintf(stderr,
			"open (\"/dev/urandom\", O_RDONLY | O_NONBLOCK): %s\n",
			strerror(errno));
		free(randstr);
		return (NULL);
	}

	if ((read(randfile, randstr, size)) < size) {
		fprintf(stderr, "read (randfile, randstr, %d): %s\n", size,
			strerror(errno));
		free(randstr);
		close(randfile);
		return (NULL);
	}

	if ((close(randfile)) != 0) {
		fprintf(stderr, "close (%d): %s\n", randfile,
			strerror(errno));
		free(randstr);
		return (NULL);
	}

	return (randstr);
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
		fprintf(stderr, "calloc (1, %d): %s\n",
			sizeof(struct sigaction), strerror(errno));
		return (errno);
	}

	my_action->sa_handler = &my_sahandler;

	if (
	    (old_action =
	     (struct sigaction *) calloc(1,
					 sizeof(struct sigaction))) ==
	    NULL) {
		fprintf(stderr, "calloc (1, %d): %s\n",
			sizeof(struct sigaction), strerror(errno));
		return (errno);
	}

	sigaction(SIGHUP, my_action, old_action);
	sigaction(SIGINT, my_action, old_action);
	sigaction(SIGQUIT, my_action, old_action);

	return (0);
}

void my_sahandler(int whatever)
{
	fprintf(stderr, "my_sahandler: received signal %d\n", whatever);
	unload_module();
	exit(whatever);
}

int big_fifo_test(int test_nr)
{
	char *filename;
	int fifo_size = 0, buf_size = 0, retval = 0;
	char *teststr, *resstr;

	if ((filename = construct_filename(test_nr)) == NULL)
		return (-1);

	fprintf(stderr, "Testing %s ", filename);

	if ((stat_test(filename)) != 0) {
		free(filename);
		return (errno);
	}
	fprintf(stderr, ". ");

	/* attempt to open while the module is not loaded; this should 
	 * fail internally */
	if ((open_test_fail(filename)) != 0) {
		free(filename);
		return (errno);
	}
	fprintf(stderr, ". ");

	/* inner loop to test different FIFO sizes */
	for (fifo_size = MIN_SIZE; fifo_size <= MAX_SIZE;
	     fifo_size *= MUL_SIZE) {
		/* now we try to load the module */
		if ((load_module(fifo_size, test_nr)) != 0) {
			free(filename);
			return (-1);
		}

		/* one last inner loop to test different read/write
		 * sizes to/from FIFO */
		for (buf_size = 1; buf_size <= fifo_size;
		     buf_size *= MUL_SIZE) {
			/* write more than the size of the FIFO; should
			 * fail internally */
			if ((over_write_test(filename, fifo_size)) != 0) {
				unload_module();
				free(filename);
				return (-1);
			}

			/* read more than the size of the FIFO; should
			 * fail internally */
			if ((over_read_test(filename, fifo_size)) != 0) {
				unload_module();
				free(filename);
				return (-1);
			}

			/* get a random string for writing to the
			 * FIFO */
			if ((teststr = get_random_str(fifo_size)) == NULL) {
				unload_module();
				free(filename);
				return (errno);
			}

			/* write to the FIFO in buf_size chunks */
			if (
			    (write_test
			     (filename, teststr, buf_size,
			      fifo_size)) != 0) {
				unload_module();
				free(filename);
				free(teststr);
				return (errno);
			}

			/* and read from the FIFO in buf_size chunks */
			if (
			    (resstr =
			     read_test(filename, buf_size,
				       fifo_size)) == NULL) {
				unload_module();
				free(filename);
				free(teststr);
				return (errno);
			}

			/* compare string written to string read to
			 * make sure they are the same */
			if ((retval = memcmp(resstr, teststr, fifo_size))
			    != 0) {
				fprintf(stderr,
					"strings not the same: %s\t%s!\n",
					resstr, teststr);
				unload_module();
				free(filename);
				free(teststr);
				free(resstr);
				return (-1);
			}

			free(teststr);
			free(resstr);

			/* and do one last write/read test which
			 * does its own internal string generation
			 * and comparison */
			if ((write_read_test(filename, buf_size)) != 0) {
				unload_module();
				free(filename);
				return (errno);
			}
		}		/* END buf_size loop */

		/* now we try to unload the module */
		if ((unload_module()) != 0) {
			free(filename);
			return (-1);
		}
		fprintf(stderr, "%d ", fifo_size);
	}			/* END fifo_size loop */
	fprintf(stderr, "passed.\n");
	free(filename);

	return 0;
}				/* END all FIFOs loop */
