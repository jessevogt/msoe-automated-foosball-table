#include <rtl.h>
#include <time.h>
#include <rtl_fifo.h>
#include <pthread.h>
#include <unistd.h>

static pthread_t thread;
static int fd_fifo;

static void * start_routine(void *arg)
{
	struct sched_param p;
	p . sched_priority = 1;
	pthread_setschedparam (pthread_self(), SCHED_FIFO, &p);

	pthread_make_periodic_np (pthread_self(), gethrtime(), 500000000);

	while (1) {
		pthread_wait_np ();
		write(fd_fifo, "test  ", 6);
	}
	return 0;
}

int init_module(void) {
	fd_fifo = open("/dev/rtf0", O_NONBLOCK | O_CREAT);
	if (fd_fifo < 0) {
		rtl_printf("/dev/rtf0 open returned %d\n", fd_fifo);
		return -1;
	}
	rtl_printf("ioctl on /dev/rtf0 (setting fifo size to 4000) returned %d\n",
			ioctl(fd_fifo, RTF_SETSIZE, 4000));

	return pthread_create (&thread, NULL, start_routine, 0);
}

void cleanup_module(void)
{
	int ret;
	pthread_delete_np (thread);
	ret = close(fd_fifo);
	rtl_printf ("rtl_fifo: close returned %d\n", ret);
}
