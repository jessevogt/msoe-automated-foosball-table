/**
 * rt_com POSIX interface
 * ======================
 *
 * RT-Linux kernel module for communication across serial lines.
 *
 * Copyright (C) 1999 Michael Barabanov <baraban@fsmlabs.com>
 */


#include <pthread.h>
#include <time.h>
#include <unistd.h>

#include <asm/io.h>

#include <rtl.h>
#include <rtl_fifo.h>

#include "rt_com.h"


#define RT_COM_POSIX_NAME

int period = 100000000;
pthread_t thread;
int fd;

void *thread_code( void *t )
{
	pthread_make_periodic_np( thread, gethrtime(), period );
	do {
		int n;
		char buf[210];
		pthread_wait_np();
		n = read( fd, buf, sizeof(buf) );
		if( n > 0 ) {
			buf[n] = 0;
			rtl_printf( "%s", buf );
		}
		write( fd, "test\n", 5 );
	} while( 1 );
	return( 0 );
}



int init_module( void )
{
	pthread_attr_t attr;
	struct sched_param sched_param;
	int thread_status;

	fd = open( "/dev/ttyS0", O_NONBLOCK );
	if( fd < 0 ) {
		rtl_printf( RT_COM_POSIX_NAME "/dev/ttyS0 open returned %d\n", fd );
		return( -1 );
	}
	rt_com_set_param( 0, 0, 0 );
	rt_com_setup( 0, 38400, RT_COM_PARITY_NONE, 1, 8 );
	pthread_attr_init( &attr );
	pthread_attr_setcpu_np( &attr, 0 );
	sched_param.sched_priority = 1;
	pthread_attr_setschedparam( &attr, &sched_param );
	thread_status = pthread_create( &thread,  &attr, thread_code, (void *)1 );
	if( thread_status < 0 ) {
		printk( RT_COM_POSIX_NAME "failed to create RT-thread\n" );
		return( -1 );
	} else {
		printk( RT_COM_POSIX_NAME "created RT-thread\n" );
	}
	return( 0 );
}


void cleanup_module( void )
{
	printk( RT_COM_POSIX_NAME "Removing module on CPU %d\n", rtl_getcpuid() );
	pthread_delete_np (thread);
	rt_com_setup(0, -1, 0, 0, 0);
}




/**
 * Local Variables:
 * mode: C
 * c-file-style: "Stroustrup"
 * End:
 */
