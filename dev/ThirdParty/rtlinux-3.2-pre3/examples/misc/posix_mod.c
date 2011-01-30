#include <rtl.h>
#include <rtl_fifo.h>
#include <time.h>
#include <rtl_sched.h>
#include <rtl_sync.h>
#include <pthread.h>
#include <posix/unistd.h>

pthread_t T;
void *my_code(void *);
int fd;
int stop = 0;
int fifo_size=4000;
static void copy_device_data(unsigned int *);

#define DELAY_NS 500000 // 500 microseconds

int init_module(void){

	rtf_create(0,fifo_size);	
	if ( (fd = open("/dev/rtf0",O_WRONLY | O_NONBLOCK )) < 0)
	{
		rtl_printf("Example cannot open fifo\n");
		rtl_printf("Error number is %d\n",errno);
		return -1;
	}
	if( pthread_create(&T,NULL,my_code,NULL))
	{
		close(fd);
		rtl_printf("Cannot create thread\n");
		return -1;
	}
	return 0;
}

void cleanup_module(void){ 
	stop = 1;
	pthread_join(T,NULL);
	close(fd);
}

void *my_code(void *arg){
	struct timespec t;
	struct {int i; unsigned int d; }D = {0,0};
	clock_gettime(CLOCK_REALTIME,&t);
	while(!stop){
		copy_device_data(&D.d);
		D.i++;
		/* ignore write fails, we just drop the data */
		write(fd,&D,sizeof(D));
		timespec_add_ns(&t,DELAY_NS);
		clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &t, NULL);
	}
	return (void *)stop;
}

static void copy_device_data(unsigned int *x)
{
	static int last=0;
	int d;
	rdtscl(d);
	*x= (d - last);
	last = d;
}

