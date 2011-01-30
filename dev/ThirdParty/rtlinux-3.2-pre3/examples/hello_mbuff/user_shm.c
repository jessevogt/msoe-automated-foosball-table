/*
 * Written by Der Herr Hofrat, der.herr@hofr.at
 * (C) 2002 FSMLabs
 * License: GPL Version 2
 */
/*
 * user space app to write to an rt-thread via shared memory (mbuff)
 */
#include <stdio.h>
#include "hello_shm.h"

int 
main(int argc, char **argv)
{
	hello_shm = (volatile char*) mbuff_alloc("hello_shm",1024);
	if( hello_shm == NULL) {
		printf("mbuff_alloc failed\n");
		return -1;	
	}
	if(argc == 2){
		sprintf((char*)hello_shm,argv[1]);
		}
	else{
		sprintf((char*)hello_shm,"good by crule world");
		}
	printf("wrote `%s` to hello_shm\n", hello_shm);

	mbuff_free("hello_shm",(void*)hello_shm);
	return 0;
}
