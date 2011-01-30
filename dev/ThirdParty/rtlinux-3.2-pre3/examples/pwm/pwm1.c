/* vim: set ts=4: */
/*
 * Copywrite 2002 Der Herr Hofrat
 * License GPL V2
 * Author der.herr@hofr.at
 */
/*
 * example of a simple PWM
 */
#include <rtl.h>
#include <rtl_fifo.h>
#include <time.h>
#include <rtl_sched.h>
#include <rtl_sync.h>
#include <pthread.h>
#include <posix/unistd.h>
#include <asm/io.h> /* outb */

#define LPT 0x378
#define LPT_CNTRL LPT+2 

pthread_t pwm_thread;
void *pwm_code(void *);
int fd;
int stop = 0;
unsigned long duty_cycle=300000;


#define DELAY_NS 500000 // 500 microseconds

void *pwm_code(void *arg){
	struct timespec t;
	unsigned char pins=0xff;
	clock_gettime(CLOCK_REALTIME,&t);
	while(!stop){
		outb(pins,LPT);
		pins = ~pins;
		timespec_add_ns(&t,DELAY_NS-duty_cycle);
		clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &t, NULL);
		outb(pins,LPT);
		pins = ~pins;
		timespec_add_ns(&t,duty_cycle);
		clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &t, NULL);
	}
	return (void *)stop;
}

int param_handler(unsigned int fifo)
{
	int err;

	while((err = rtf_get(0, &duty_cycle, sizeof(duty_cycle))) == sizeof(duty_cycle)){
		rtl_printf("duty cycle set to %d\n",(int)duty_cycle);
	}
	if (err != 0) {
		return -EINVAL;
	}
	return 0;
}

int init_module(void){

	rtf_create(0,4000);	
	if ( (fd = open("/dev/rtf0",O_RDONLY | O_NONBLOCK )) < 0)
	{
		printk("Open fifo failed %d\n",errno);
		return -1;
	}
	if(pthread_create(&pwm_thread,NULL,pwm_code,NULL))
	{
		close(fd);
		printk("Pthread_create failed\n");
		return -1; 
	}
	rtf_create_handler(0, &param_handler); 
	return 0;
}

void cleanup_module(void){ 
	stop = 1;
	pthread_join(pwm_thread,NULL);
	close(fd);
}

