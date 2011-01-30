/*
 * Copywrite 2002 Der Herr Hofrat
 * License GPL V2
 * Author der.herr@hofr.at
 */
/*
 * simple read of a fifo in which the rt-task is dumping gethrtime data
 */

#include <stdio.h>     /* fprintf */
#include <stdlib.h>    /* exit */
#include <sys/types.h> /* open */
#include <fcntl.h>     /* open */
#include <unistd.h>    /* close */ 

int 
main(int argc,char ** argv)
{
	int fd;
	int n;
	unsigned long long rt_time;
	
	if ((fd = open("/dev/rtf0", O_RDONLY)) < 0) {
		fprintf(stderr, "Error opening /dev/rtf0\n");
		exit(1);
	}

	while(1){
		n = read(fd, &rt_time, sizeof(rt_time));
		printf("got: %16lld\n", (unsigned long long) rt_time);
		fflush(stdout);
	}

	close(fd);

	return 0;
}
