#include <rtl.h>
#include <time.h>
#include <pthread.h>
#include <asm/io.h>
#include <rtl_fifo.h>

#define OUT_FIFO_ID 2
#define OUT_FIFO_LENGTH 0x100

pthread_t thread;

void * thread_code(void)
{
	pthread_make_periodic_np(pthread_self(), gethrtime(), 1);

		
	outb(0x00,0x350+3);
	outb(0x00,0x350+2);

	while(1)
	{
		//pthread_wait_np();
		//rtl_printf("hello world\n");
		//outb(0xFF,0x350+2);
		//nanosleep(hrt2ts(36000),NULL);
		//outb(0x00,0x350+2);
		char data = 'X';
		rtf_put(OUT_FIFO_ID,&data,sizeof(char));
		nanosleep(hrt2ts(10000000),NULL);
	}

	return 0;
}

int init_module(void)
{
	rtf_destroy(OUT_FIFO_ID);

	rtf_create(OUT_FIFO_ID, OUT_FIFO_LENGTH);
	return pthread_create(&thread, NULL, thread_code, NULL);
}

void cleanup_module(void)
{
	pthread_delete_np(thread);
	rtf_destroy(OUT_FIFO_ID);
}

