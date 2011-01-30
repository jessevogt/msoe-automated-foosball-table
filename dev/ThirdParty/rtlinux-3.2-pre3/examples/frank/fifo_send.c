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

int main()
{
	int fd0;
	int msg=0;
	
	if ((fd0 = open("/dev/rtf1", O_RDWR)) < 0) {
		fprintf(stderr, "Error opening /dev/rtf1\n");
		exit(1);
	}

   printf("sending to /dev/rtf0 - <CNTRL>-<C> to stop\n");
   while (1) {
	   if (write(fd0, &msg, sizeof(msg)) < 0) {
		  fprintf(stderr, "Can't send a command to RT-task\n");
		  exit(1);
      }
      printf(".");
      fflush(stdout);
      sleep(1);
	}
	return 0;
}
