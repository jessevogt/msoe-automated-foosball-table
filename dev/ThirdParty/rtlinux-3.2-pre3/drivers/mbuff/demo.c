#include <stdio.h>

#include "mbuff.h"

/* the contents of shared memory may change at any time, thus volatile */
volatile char * shm1, *shm2;

main (int argc,char *argv[]){
   
	shm1 = (volatile char*) mbuff_alloc("demo1",1024*1024);
	shm2 = (volatile char*) mbuff_alloc("demo1",1024*1024);
	if( shm1 == NULL || shm2 == NULL ) {
		printf("mbuff_alloc failed\n");
		exit(2);
	}
	sprintf((char*)shm1,"example data\n");
	sleep(5); /* you may change it from the kernel or other program here */
	printf("shm1=%p shm2=%p shm2->%s", shm1, shm2, shm2);
	mbuff_free("demo1",(void*)shm1);
	sleep(3);
	/* you may still access shm2 here, it is still the same memory area */
	mbuff_free("demo1",(void*)shm2);
	return(0);
}
