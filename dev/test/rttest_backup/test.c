#include <rtl.h>
#include <time.h>
#include <pthread.h>
#include <asm/io.h>

pthread_t thread1;
pthread_t thread2;
pthread_t thread3;
pthread_t thread4;

const int BASE = 0x350;
const int [] PORTS = {BASE + 0, BASE + 1, BASE + 4, BASE + 5, BASE + 6};

void * thread_code()
{
	int pin = 0x350 + 2;

	pthread_make_periodic_np(pthread_self(), gethrtime(), 1);

		
	outb(0x00,0x350+3);
	outb(0x00,pin);

	while(1)
	{
		//pthread_wait_np();
		//rtl_printf("hello world\n");
		outb(0xFF,pin);
		nanosleep(hrt2ts(36000),NULL);
		outb(0x00,pin);
		nanosleep(hrt2ts(36000),NULL);
	}

	return 0;
}

int init_module(void)
{
	pthread_create(&thread1, NULL, thread_code(1), NULL);
//	pthread_create(&thread2, NULL, thread_code, NULL);
//	pthread_create(&thread3, NULL, thread_code, NULL);
//	pthread_create(&thread4, NULL, thread_code, NULL);
	return 0;
}

void cleanup_module(void)
{
	pthread_delete_np(thread1);
}

