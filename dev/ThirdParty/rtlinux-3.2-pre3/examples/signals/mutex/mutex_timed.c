/*
 * Added user signals to timed mutexes test,
 * Dec, 2002 Josep Vidal <jvidal@disca.upv.es> (OCERA)
 */

#include <rtl.h>
#include <time.h>
#include <pthread.h>
#define ITERS 1
#define NTASKS 2
static pthread_t thread[NTASKS];

static pthread_mutex_t mutex /* = PTHREAD_MUTEX_INITIALIZER */;
static int count=0;


void sig_hdl(int sig){
  
  rtl_printf("Hi, I am thread %d, really blocked on a timed mutex\n",sig - RTL_SIGUSR1);
  rtl_printf("But signal handlers are executed regardless being blocked on a mutex\n");
  rtl_printf("After finishing the execution of this handler I will remain blocked on the mutex\n");

}

static void * start_routine(void *arg)
{
	int ret,i,signal,err=0;
	int nthread = (int) arg;
	struct sigaction sa;
	struct timespec maxtime;
	rtl_sigset_t mask;

	struct sched_param p;
	p . sched_priority = 1;
	pthread_setschedparam (pthread_self(), SCHED_FIFO, &p);

	maxtime.tv_sec=0;
	maxtime.tv_nsec=100*1000*1000;

	rtl_printf("thread %d starts on CPU%d\n", nthread, rtl_getcpuid());
	ret = pthread_mutex_trylock(&mutex);
	rtl_printf ("thread %d: pthread_mutex_trylock returned %d\n", nthread, ret);
	
		/* This code will be executed by thread with nthread == o */
	if (nthread==0){
	  rtl_printf("THREAD %d, sending signals to others thread blocked on the mutex count:%d\n",nthread,count);
	  nanosleep(hrt2ts(100*1000),NULL);
	  for (i=1;i<NTASKS;i++){
	    ret=pthread_kill(thread[i],RTL_SIGUSR1+i);
	    rtl_printf("pthread_kill returns %d when killing thread: %d\n",ret,i);
	  }
	} else {
	  /* This code will be executed by thread with nthread != o */
	  signal=RTL_SIGUSR1+nthread;
	  sa.sa_handler=sig_hdl;
	  if ((err=sigaction(signal,&sa,NULL))<0 ){
	    rtl_printf("thread %d -> sigaction(%d,&sa,NULL) FAILING, err:%d.\n",nthread,signal,err);
	  }
	  
	  rtl_sigfillset(&mask);
	  rtl_sigdelset(&mask,signal);
	  
	  pthread_sigmask(SIG_SETMASK,&mask,NULL);
	  
	  
	  if (ret != 0) {
	    rtl_printf("thread %d: about to pthread_mutex_timedlock\n", nthread);
	    ret = pthread_mutex_timedlock (&mutex,hrt2ts(gethrtime()+timespec_to_ns(&maxtime)));
	    rtl_printf("thread %d: pthread_mutex_timedlock returned %d\n", nthread, ret);
	  }
	  
	  if (ret==ETIMEDOUT){
	    rtl_printf("TEST SUCCESFULY PASSED for thread %d\n",nthread);
	  }
	}	
	
	nanosleep(&maxtime,NULL);
	ret = pthread_mutex_unlock (&mutex);
	rtl_printf("thread %d: pthread_mutex_unlock returned %d\n", nthread, ret);

	return (void *) (35 + nthread);
}


int init_module(void)
{
	int ret;
	int i;
	pthread_attr_t attr;

	pthread_mutex_init (&mutex, 0);

	rtl_printf("RTLinux mutex test starts on CPU%d\n", rtl_getcpuid());
	pthread_attr_init (&attr);
	for (i = 0; i < NTASKS; i++) {
		/* try to run one thread on another CPU */
		if (i == 1 && rtl_cpu_exists(!rtl_getcpuid())) {
			pthread_attr_setcpu_np(&attr, !rtl_getcpuid());
		}

		ret = pthread_create (&thread[i], &attr, start_routine, (void *) i);
		if (ret) {
			rtl_printf("failed to create a thread\n");
			return ret;
		}
	}


	return 0;
}

void cleanup_module(void) {
  int i;
  void *retval;

  for (i=0;i<NTASKS;i++){
    pthread_join (thread[i], &retval);
    rtl_printf("pthread_join on thread %d returned %d\n", i, (int) retval);
    
    //pthread_delete_np(thread[i]);
  }

  pthread_mutex_destroy(&mutex);

}
