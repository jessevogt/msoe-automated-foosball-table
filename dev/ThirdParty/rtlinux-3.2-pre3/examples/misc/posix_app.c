#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define NANOSECONDS_PER_TICK (double)((double)1000.00/(double)366.00) 

int main(int argc, char ** argv){
	int fd;
	int count=0 ;
	struct { int count; unsigned int d;}D;
	double f;

	if( (fd = open("/dev/rtf0",O_RDONLY) ) < 0 ){
		perror("Can't open fifo");
		exit(0);
	}

	while( (read(fd,&D,sizeof(D))  == sizeof(D))){
		f = (double)D.d;
		f = ((f) * NANOSECONDS_PER_TICK)/1000;
		printf("Delay was %f microseconds\n",f);
		if(count && (count+1 != D.count))
			printf("Dropped a packet at %d ",count);
		count = D.count;
	}
	
	close(fd);
	return 0;
}


