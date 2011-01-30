/*
 * Written by Der Herr Hofrat, der.herr@hofr.at
 * (C) 2002 FSMLabs
 * License: GPL Version 2
 */
#include <rtl.h>
#include <time.h>
#include <rtl_debug.h>
#include <errno.h>
#include <pthread.h>
#include "hello.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

pthread_t thread;

extern void * start_routine(void *arg);
struct shared_mem_struct* shared_mem;
int memfd;

int 
init_module(void) 
{
	int ret;

	memfd = open("/dev/mem", O_RDWR);
	if (memfd){
		shared_mem = (struct shared_mem_struct*) mmap(0,
	  		sizeof(struct shared_mem_struct),
	  		PROT_READ | PROT_WRITE, 
			MAP_FILE | MAP_SHARED, 
			memfd, 
			MEMORY_OFFSET);
		if(shared_mem != NULL){  	
			printk("Dev mem available\n");
		}
		else{
			printk("Failed to map memory\n");
			close (memfd);
			return -1;
		}
	}
	else{
		printk("Failed to open memory device file\n");
		return -1;
	}
	ret=pthread_create (&thread, NULL, start_routine, 0);
	return ret;
}

void cleanup_module(void) {
	pthread_delete_np (thread);
	close(memfd);
}
