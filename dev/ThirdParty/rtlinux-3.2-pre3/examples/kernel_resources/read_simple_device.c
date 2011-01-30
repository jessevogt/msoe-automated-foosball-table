#include <stdio.h>
#include <fcntl.h>
#include "device_common.h"

main(){
	int fd,n;
	char data[16];
	if((fd=open(SIMPLE_DEV, O_RDWR|O_SYNC))<0)
	{
		perror("open");
		exit(-1);
	}
	/* just read to trigger the read-fops */
	n = read(fd, &data, sizeof(data));
	return 0;
}
