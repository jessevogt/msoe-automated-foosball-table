//#include <asm/io.h>
//#include <stdio.h>
#include <sys/io.h>
#include <time.h>
#include <pthread.h>

int main(char** argv, int argc)
{
	short base = 0x350;

	iopl(3);

	outb(0,base+3);
		
	while(1)
	{
		outb(0xFF,base+2);
		nanosleep(hrt2ts(500000),NULL);
		outb(0x00,base+2);
		nanosleep(hrt2ts(500000),NULL);
	}

	return 0;
}
