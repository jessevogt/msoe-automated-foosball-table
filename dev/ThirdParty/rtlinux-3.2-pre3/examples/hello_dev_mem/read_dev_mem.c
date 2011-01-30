/*
 * Written by Der Herr Hofrat, der.herr@hofr.at
 * (C) 2002 FSMLabs
 * License: GPL Version 2
 */

/*
 *  Example of reading shared memory from user-space
 */

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "hello.h"

struct shared_mem_struct* shared_mem;
int memfd;

int main()
{
	char input[40];

	memfd = open("/dev/mem", O_RDONLY);
	if (memfd)
	{
	  shared_mem = (struct shared_mem_struct*) mmap(0,
	  	sizeof(struct shared_mem_struct),
	  	PROT_READ, MAP_FILE | MAP_SHARED, memfd, MEMORY_OFFSET);
	  if (shared_mem != NULL)
	  {  	
		  do
			{
				printf("found: %d %d\n",
					shared_mem->some_int,
					(int)shared_mem->ready);
				sleep(1);
			}
			while (1);
		}
		else
			printf("Failed to map memory\n");
		close (memfd);
	}
	else{
		printf("Failed to open memory device file\n");
		exit(-1);
		}

	close(memfd);
	return 0;
}
