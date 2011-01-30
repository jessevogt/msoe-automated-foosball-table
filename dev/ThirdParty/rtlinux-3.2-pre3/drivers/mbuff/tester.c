#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "mbuff.h"

unsigned long baseaddr=0x00;
#define DMA_TOTAL_SIZE (3*4096)
/* it could be also written as 'ab' but gcc complains */
#define DMA_FILL1 ('a'+'b'<<8)
#define DMA_FILL2 ('C'+'D'<<8)
#define DMA_FILL3 ('x'+'y'<<8) 
main (int argc,char **argv){
 int fd, fd1;
 volatile unsigned short int *dmabuf;  
 int i;
	struct mbuff_request_struct req={0,"mybuf",DMA_TOTAL_SIZE*4,0};;
   
   if(( fd = open(MBUFF_DEV_NAME,O_RDWR) ) < 0 ){
	perror("open failed");
      exit(1);
   }
   
   /*caddr_t  mmap(caddr_t  addr,  size_t  len,  int prot , int
		        flags, int fd, off_t offset );*/

	if((dmabuf=(volatile unsigned short int *)mmap(NULL,DMA_TOTAL_SIZE,PROT_WRITE|PROT_READ,MAP_SHARED|MAP_FILE,fd, 0)) == (volatile unsigned short int*) -1){
		printf("MMAP error %d\n",dmabuf);
		perror("mmap");
//		exit(2);
	} else {
		printf("success, %d bytes mmaped at %p\n",DMA_TOTAL_SIZE,dmabuf);
		sleep(2);
		for(i=0;i<DMA_TOTAL_SIZE/sizeof(*dmabuf);i++) {
			dmabuf[i]=DMA_FILL1;
		}
		printf("memory set\n");
	}
	ioctl(fd,IOCTL_MBUFF_ALLOCATE,&req);
	mmap(NULL,DMA_TOTAL_SIZE*4,PROT_WRITE|PROT_READ,MAP_SHARED|MAP_FILE,fd,0);
	ioctl(fd,IOCTL_MBUFF_DEALLOCATE,&req); /* will be freed on unmap */
	sleep(5);
	close(fd);
	printf("closed\n");
	sleep(5);
	for(i=0;i<DMA_TOTAL_SIZE/sizeof(*dmabuf);i++) {
		dmabuf[i]=DMA_FILL2;
	}
	printf("set again to CD\n");
	sleep(5);
	printf("unmapping in 1s\n");
	sleep(1);
	munmap((char*)dmabuf,DMA_TOTAL_SIZE);
	printf("unmapped\n");
	sleep(5);
	printf("trying to write again\n");
	for(i=0;i<DMA_TOTAL_SIZE/sizeof(*dmabuf);i++) {
                dmabuf[i]=DMA_FILL3;
        }

	return(0);
}
