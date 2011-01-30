/*
 * Modified by J. Vidal to test RTLinux POSIX.4 interval timers 22-09-2002 
 *
 */


#include <rtl.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include "utils.h"

#define perror(s) do { rtl_printf(s" -> error localitzation: "); myperror(s); } while(0)
#define printf rtl_printf
#define getpid() th
#define kill pthread_kill
#define sleep(sec) usleep(sec*1000*1000)
#define exit(a) do {return a;} while (0)
#define strerror(a) a
#define MAX_NOF_TIMERS 6000
#define SIGALRM RTL_SIGUSR1
#define sigprocmask pthread_sigmask
timer_t tt[MAX_NOF_TIMERS];
timer_t t,t2;
pthread_t th;
struct timespec ref;
struct itimerspec ispec;
struct itimerspec ospec;
struct sigaction sa;
rtl_sigset_t set;
int retval;
int i,j;
int signo,sival_expected;  
struct sigevent timer_event_spec;

static void reset_ref_time(void)
{
  clock_gettime(CLOCK_REALTIME, &ref);
}
/*
static void print_rel_time(void)
{
  struct timespec now;
  clock_gettime(CLOCK_REALTIME, &now);
  now.tv_sec -= ref.tv_sec;
  now.tv_nsec -= ref.tv_nsec;
  if (now.tv_nsec < 0) {
    now.tv_sec--;
    now.tv_nsec += 1000000000;
  }
  printf("%ld.%09ld", now.tv_sec, now.tv_nsec);
}
*/

void alrm_handler(int signo){
  rtl_printf("Alarm handler called for signal:%d\n",signo);
}

void *th_code(void *arg){

  rtl_printf("Starting th_code\n");

  rtl_sigemptyset(&set);
  sigprocmask(SIG_SETMASK, &set, NULL);
  
  sa.sa_handler = alrm_handler;
  rtl_sigemptyset(&sa.sa_mask);

  if (sigaction(SIGALRM, &sa, NULL)) {
    perror("sigaction failed");
    exit(1);
  }
  
  if (sigaction(RTL_SIGRTMIN, &sa, NULL)) {
    perror("sigaction failed");
    exit(1);
  }

  rtl_printf("before sigaction(%d, &sa, NULL)) \n",RTL_SIGRTMIN +1 );
  if (sigaction(RTL_SIGRTMIN + 1, &sa, NULL)) {
    perror("sigaction failed");
    exit(1);
  }
  
  printf("\ntest 10: set absolute time (no time specification): expect failure\n");
	retval = timer_settime(t, TIMER_ABSTIME, NULL, NULL);
	if (retval) {
		perror("timer_settime(TIMER_ABSTIME) failed");
	}
	assert(retval == -1);
	


	printf("\ntest 11: set relative time (no time specification): expect failure\n");
	retval = timer_settime(t, 0, NULL, NULL);
	if (retval) {
		perror("timer_settime(0) failed");
	}
	assert(retval == -1);

	retval = clock_gettime(CLOCK_REALTIME, &ispec.it_value);
	if (retval) {
		perror("clock_gettime(CLOCK_REALTIME) failed");
	}
	assert(retval == 0);
	ispec.it_value.tv_sec += 2;
	ispec.it_value.tv_nsec = 0;
	ispec.it_interval.tv_sec = 0;
	ispec.it_interval.tv_nsec = 0;
	reset_ref_time();

	printf("\ntest 18: set timer (absolute time) 2 seconds in the future\n");
	retval = timer_settime(t, TIMER_ABSTIME, &ispec, &ospec);
	if (retval) {
		perror("timer_settime(TIMER_ABSTIME) failed");
	}
	assert(retval == 0);
	printf("timer_settime: old setting value=%ld.%09ld, interval=%ld.%09ld\n",
	       ospec.it_value.tv_sec, ospec.it_value.tv_nsec,
	       ospec.it_interval.tv_sec, ospec.it_interval.tv_nsec);

	reset_ref_time();

	printf("\ntest 19: set timer (absolute time) same time\n");
	retval = timer_settime(t, TIMER_ABSTIME, &ispec, &ospec);
	if (retval) {
		perror("timer_settime(TIMER_ABSTIME) failed");
	}
	assert(retval == 0);
	printf("timer_settime: old setting value=%ld.%09ld, interval=%ld.%09ld\n",
	       ospec.it_value.tv_sec, ospec.it_value.tv_nsec,
	       ospec.it_interval.tv_sec, ospec.it_interval.tv_nsec);

	

	printf("\ntest 21: timer_gettime good timer id, NULL timespec pointer: expect failure\n");
	retval = timer_gettime(t, NULL);
	if (retval) {
		perror("timer_gettime() failed");
	}
	assert(retval == -1);

	printf("\ntest 23: timer_gettime good timer id, good timespec pointer\n");
	retval = timer_gettime(t, &ispec);
	if (retval) {
		perror("timer_gettime() failed");
	}
	assert(retval == 0);
	printf("timer_gettime: value=%ld.%09ld, interval=%ld.%09ld\n",
	       ispec.it_value.tv_sec, ispec.it_value.tv_nsec,
	       ispec.it_interval.tv_sec, ispec.it_interval.tv_nsec);


	printf("\ntest 24: send ALRM signal to self with kill()\n");
	reset_ref_time();
	sival_expected = -1;
	retval = kill(getpid(), SIGALRM);
	if (retval) {
		perror("kill(myself with SIGALRM) failed");
	}
	assert(retval == 0);
        usleep(1); // only to call the scheduler.

	rtl_sigemptyset(&set);
	rtl_sigaddset(&set, SIGALRM);
	sigprocmask(SIG_BLOCK, &set, NULL);
	
	printf("\ntest 26: send ALRM signal to self with kill() (signal blocked)\n");
	retval = kill(getpid(), SIGALRM);
	if (retval) {
		perror("kill(myself with SIGALRM) failed");
	}
	assert(retval == 0);
	usleep(1); // only to call the scheduler.


	printf("\ntest 30: timer_gettime()\n");
	sleep(1);
	retval = timer_gettime(t, &ispec);
	if (retval) {
		perror("timer_gettime() failed");
	}
	assert(retval == 0);
	printf("timer_gettime: value=%ld.%09ld, interval=%ld.%09ld\n",
	       ispec.it_value.tv_sec, ispec.it_value.tv_nsec,
	       ispec.it_interval.tv_sec, ispec.it_interval.tv_nsec);

	rtl_printf(" wait for timer expiration of test 18 \n");
	sleep(1);		/* wait for timer expiration of test 18 */


	printf("\ntest 32: timer_gettime: deleted timer and NULL itimer_spec: expect failure\n");
	retval = timer_gettime(tt[1], NULL);
	if (retval) {
		perror("timer_gettime() failed");
	}
	assert(retval == -1);

	/*
	 * Test to see if timer goes off immediately if not a future time is
	 * provided with TIMER_ABSTIME 
	 */
	printf("\ntest 35: set up timer to go off immediately, followed by 10 ticks at 10 Hz\n");
	ispec.it_value.tv_nsec = 1;
	rtl_printf("ispec.it_value.tv_nsec:%d\n",ispec.it_value.tv_sec);
	ispec.it_interval.tv_sec = 0;
	ispec.it_interval.tv_nsec = 100000000;
	reset_ref_time();
	retval = timer_settime(t2, TIMER_ABSTIME, &ispec, &ospec);
	if (retval) {
		perror("timer_settime(TIMER_ABSTIME) failed");
	}

	printf("timer should have expired now\n");
	printf("timer_settime: old setting value=%ld.%09ld, interval=%ld.%09ld\n",
	       ospec.it_value.tv_sec, ospec.it_value.tv_nsec,
	       ospec.it_interval.tv_sec, ospec.it_interval.tv_nsec);
	retval = timer_gettime(t, &ispec);
	if (retval) {
		perror("timer_gettime() failed");
	}
	printf("timer_gettime: value=%ld.%09ld, interval=%ld.%09ld\n",
	       ispec.it_value.tv_sec, ispec.it_value.tv_nsec,
	       ispec.it_interval.tv_sec, ispec.it_interval.tv_nsec);
	printf("catch 10 signals\n");
	rtl_sigfillset(&set);
	rtl_sigdelset(&set,RTL_SIGRTMIN);
	for (i = 0; i < 10; i++) {
	  sigsuspend(&set);
	  rtl_printf("Number of signals catched %d\n",i);
	}

	pthread_exit(NULL);
	return 0;

}

int init_module(void)
{
	printf("\ntest 7: attempt to create too many timers: don't expect failure in RTLinux while there is free memory\n");
	for (i = 0; i < MAX_NOF_TIMERS; i++) {
	  	retval = timer_create(CLOCK_REALTIME, NULL, &tt[i]);
		if (retval) {
			rtl_printf("timer_create(CLOCK_REALTIME) %d failed: %d\n", i, strerror(errno));
			break;
		}
	}
        if ( i < MAX_NOF_TIMERS) {
                assert((i > 30) && (retval == -1));
                j = i;
        }else {
                assert( (i == MAX_NOF_TIMERS) && (retval != -1));
                rtl_printf( "timer_create(CLOCK_REALTIME) %d timers created\n",i);
                j = i ;
        }

	printf("\ntest 8: delete these timers: don't expect failure in RTLinux\n");
	for (i = 0; i < j; i++) {
	  	retval = timer_delete(tt[i]);
		if (retval) {
			rtl_printf("timer_delete(CLOCK_REALTIME) %d failed: %d\n", i, strerror(errno));
			break;
		}
	}
	assert(retval == 0);

	printf("\ntest 9: create default timer\n");
	retval = timer_create(CLOCK_REALTIME, NULL, &t);
	if (retval) {
		perror("timer_create(CLOCK_REALTIME) failed");
	}
	assert(retval == 0);

	
	printf("\ntest 31: timer_delete()\n");
	retval = timer_delete(t);
	if (retval) {
		perror("timer_delete(deleted timer) failed");
	}
	assert(retval == 0);


	/*
	 * test to check timer cancellation by deletion
	 */

	printf("\ntest 33: create default timer\n");
	retval = timer_create(CLOCK_REALTIME, NULL, &t);
	if (retval) {
		perror("timer_create(CLOCK_REALTIME) failed");
	}
	
	retval = clock_gettime(CLOCK_REALTIME, &ispec.it_value);
	if (retval) {
	  perror("clock_gettime(CLOCK_REALTIME) failed");
	}

	/*
         * Test to see if timer goes off immediately if not a future time is
         * provided with TIMER_ABSTIME
         */
        printf("\ntest 35: set up timer to go off immediately, followed by 10 ticks at 10 Hz\n");
        timer_event_spec.sigev_notify = SIGEV_SIGNAL;
        timer_event_spec.sigev_signo = RTL_SIGRTMIN + 0;
        sival_expected = timer_event_spec.sigev_value.sival_int = 0x1234;
        retval = timer_create(CLOCK_REALTIME, &timer_event_spec, &t2);
	
        retval = clock_gettime(CLOCK_REALTIME, &ispec.it_value);
        if (retval) {
          perror("clock_gettime(CLOCK_REALTIME) failed");
        }

	rtl_printf("Create test thread.\n");
	pthread_create(&th,NULL,th_code,(int *) 0);

	return 0;

}

void cleanup_module(void){
  int retval;
  
printf("\ntest 31: timer_delete()\n");
  retval = timer_delete(t);
  if (retval) {
    perror("timer_delete(existing timer) failed");
  }
  assert(retval == 0);  
  retval = timer_delete(t2);
  if (retval) {
    perror("timer_delete(existing timer) failed");
  }
  assert(retval == 0);

  printf("\ntest 1: delete non existing timer: expect failure\n");
  retval = timer_delete((PAGE_OFFSET+10000));
  if (retval) {
    perror("timer_delete(bogus timer) failed");
  }
  assert(retval == -1);
  
  
  printf("\ntest 4: delete timer again: expect failure\n");
  retval = timer_delete(t);
  if (retval) {
    perror("timer_delete(deleted timer) failed");
  }
  assert(retval == -1);

  printf("\ntest 5: delete non-existing timer: expect failure\n");
  retval = timer_delete(0);
  if (retval) {
    perror("timer_delete(0) failed");
  }
  assert(retval == -1);
  
  rtl_printf("\nDeleting test thread\n");
  pthread_delete_np(th);
  
}











