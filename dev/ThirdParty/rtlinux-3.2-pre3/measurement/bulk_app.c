#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <rtl_fifo.h>
#include <rtl_time.h>
#include "bulk.h"


struct timeval t,t2;
int buf[BUF_SIZE];
int main()
{
	int fdr;
	int fdw;
	int n,i;
	unsigned long long  us =0,mbps;
	
	if ((fdr = open("/dev/rtf1", O_RDONLY)) < 0) {
		fprintf(stderr, "Error opening /dev/rtf1\n");
		exit(1);
	}
	if ((fdw = open("/dev/rtf2", O_WRONLY)) < 0) {
		fprintf(stderr, "Error opening /dev/rtf2\n");
		exit(1);
	}

	gettimeofday(&t,0);
	for (i = 0; i < COUNT* (BUF_SIZE* sizeof(int)); i+=n) {
	    n = read(fdr, buf, sizeof(buf));
	    if(n <0){
		    printf("Error in reading %d\n",n);
		    break;
	    }
	}
	gettimeofday(&t2,0);
	us = ((t2.tv_sec - t.tv_sec)*1000000) + t2.tv_usec;
	us -= t.tv_usec;
	mbps = i/us;
        printf("Read %d bytes in %Ld microseconds = %Ld Mb/second \n",i,us,mbps);
	fflush(stdout);


	gettimeofday(&t,0);
	for (i = 0; i < COUNT* (BUF_SIZE* sizeof(int)); i+=n) {
	    n = write(fdw, buf, sizeof(buf));
	    if(n <0){
		    printf("Error in writing %d\n",n);
		    break;
	    }
	}
	gettimeofday(&t2,0);
	us = (t2.tv_sec*1000000) + t2.tv_usec;
	us -= ((t.tv_sec*1000000) + t.tv_usec);
	mbps = i/us;
        printf("Wrote %d bytes in %Ld microseconds = %Ld Mb/second \n",i,us,mbps);


	return 0;

}



