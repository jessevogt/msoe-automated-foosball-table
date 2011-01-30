/*
 * Added user signals to mutex test,
 * Dec, 2002 Josep Vidal <jvidal@disca.upv.es> (OCERA)
 */

#include <rtl.h>
#include <time.h>
#include <pthread.h>
#define NTASKS 3
#define ITERS 2
static pthread_t threads[NTASKS];

static pthread_mutex_t mutex /* = PTHREAD_MUTEX_INITIALIZER */;

void my_sig_handler(int sig_rec){
  int thread_no;
  
  thread_no=sig_rec-RTL_SIGRTMIN;
  rtl_printf("I'm thread %d, really blocked on a mutex...\n",thread_no);
  rtl_printf("but signals handlers are executed regardless being blocked on a mutex\n");
  rtl_printf("After finishing handler execution for signal:%d, I will remain blocked on the mutex\n",sig_rec);
  
}

static void * start_routine(void *arg)
{
  int ret,i=0,j,signal;
  hrtime_t t;
  hrtime_t t2;
  int sleep = 500000000;
  int nthread = (int) arg;
  struct sigaction sa;
  rtl_sigset_t mask;
  
  struct sched_param p;
  p . sched_priority = nthread;
  pthread_setschedparam (pthread_self(), SCHED_FIFO, &p);
  
  
  // Block all signals except the ones you want to receive.
  if (nthread){
    sa.sa_handler=my_sig_handler;
    sa.sa_mask=0;
    sa.sa_flags=0;
    
    rtl_sigfillset(&mask);
    signal=RTL_SIGRTMIN+nthread;
    rtl_sigdelset(&mask,signal);
    pthread_sigmask(SIG_SETMASK,&mask,NULL);
    
    if ((ret=sigaction(signal,&sa,NULL))<0 ){
      rtl_printf("sigaction(%d,&sa,NULL) FAILING, ret:%d.\n",signal,ret);
    }
  }
  
  rtl_printf("thread %d starts on CPU%d\n", nthread, rtl_getcpuid());
  ret = pthread_mutex_trylock(&mutex);
  rtl_printf ("thread %d: pthread_mutex_trylock returned %d\n", nthread, ret);
  
  if (ret != 0) {
    rtl_printf("thread %d: about to pthread_mutex_lock\n", nthread);
    t = gethrtime();
    ret = pthread_mutex_lock (&mutex);
    t2 = gethrtime();
    rtl_printf("thread %d: pthread_mutex_lock returned %d (%d ns elapsed)\n", nthread, ret, (unsigned) (t2 - t));
  }
  
  rtl_printf("thread %d is about to sleep for %d ns\n", nthread, sleep);
  while (i++ < ITERS){
    nanosleep (hrt2ts(sleep), NULL);
    /*
      Send all programmed signals to all threads but the sender.
      Only the thread which has that signal unblocked will receive it.
      Also only the signal which has programmed
      a handler to that signal will execute it.
    */
    if (!nthread){
      rtl_printf("thread %d testing mutexes in iter %d\n",nthread,i);
      for (i=RTL_SIGRTMIN+1;i<(RTL_SIGRTMIN+NTASKS);i++)
	for (j=1;j<NTASKS;j++){
	  ret=pthread_kill(threads[j],i);
	  //	      rtl_printf("pthread_kill(%d,%d) returns %d\n",j,i,ret);
	}
    }
  }
  
  ret = pthread_mutex_unlock (&mutex);
  
  rtl_printf("thread %d: pthread_mutex_unlock returned %d\n", nthread, ret);
  
  return (void *) 35 + nthread;
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
    
    ret = pthread_create (&threads[i], &attr, start_routine, (void *) i);
    if (ret) {
      rtl_printf("failed to create a thread\n");
      return ret;
    }
  }
  
  return 0;
}


void cleanup_module(void)
{
  void *retval;
  int i;
  for (i = 0; i < NTASKS; i++) {
    pthread_join (threads[i], &retval);
    rtl_printf("pthread_join on thread %d returned %d\n", i, (int) retval);
  }
  pthread_mutex_destroy (&mutex);
}
