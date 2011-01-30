/*
 * Written by Der Herr Hofrat, der.herr@hofr.at
 * (C) 2002 FSMLabs
 * License: GPL Version 2
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h> /* exit */
#include <sys/io.h>
#define LPT 0x378

int main(void)
{
	int in;
	if (ioperm(LPT, 3 , 1) < 0) {
		fprintf(stderr,"ioperm: error accessing to IO-ports. Root Privileges required");
		exit(-1);
	}


	while(1) {
		in=inb(LPT+1);
		in = in >> 3;
		in = in & 0x0f;
		printf("%x\n",in);
		usleep(100000);     
	}
	exit(0);
}
