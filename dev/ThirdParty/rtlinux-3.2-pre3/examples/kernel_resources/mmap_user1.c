/*
 * Written by Der Herr Hofrat, der.herr@hofr.at
 * (C) 2002 FSMLabs
 * License: GPL Version 2
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "device_common.h"

int 
main(void)
{
	int fd;
	char msg[LEN];
	unsigned int *addr;

	if((fd=open(SIMPLE_DEV, O_RDWR|O_SYNC))<0)
	{
		perror("open");
		exit(-1);
	}
	addr = mmap(0, LEN, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

	printf("enter a short test:");
	scanf("%s",&msg);
  
	if(!addr)
	{
		perror("mmap");
		exit(-1);
	}
	else
	{
		memset(addr,0,LEN);
		strncpy((char *)addr,msg,sizeof(msg));
		printf("Put: %s\n",addr);
	}
	munmap(addr,LEN);
	close(fd);
	return 0;
}
