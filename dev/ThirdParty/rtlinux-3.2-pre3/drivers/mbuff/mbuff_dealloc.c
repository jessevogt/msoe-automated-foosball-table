#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "mbuff.h"

struct mbuff_request_struct req={0,"mybuf",0,0};;
void usage(char * program) {
	printf("usage: %s <shm_name>\n<shm_name> is max %d characters long\n",
		program,sizeof(req.name)-1);
}
main (int argc,char **argv){
	int fd;
   
	if(argc != 2 ) {
		usage(argv[0]);
		exit(1);
	}
	strncpy(req.name,argv[1],sizeof(req.name));
	req.name[sizeof(req.name)-1]='\0';
	if(( fd = open(MBUFF_DEV_NAME,O_RDWR) ) < 0 ){
		perror("open failed");
		exit(2);
	}
	printf("%s shared memory area was %d bytes long\n",req.name,
		ioctl(fd,IOCTL_MBUFF_DEALLOCATE,&req));
	return(0);
}
