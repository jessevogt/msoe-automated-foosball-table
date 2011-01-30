/*
 * Added user signals to timed semaphores test,
 * Dec, 2002 Josep Vidal <jvidal@disca.upv.es> (OCERA)
 */

#include <rtl.h>
#include <rtl_sema.h>
#include <rtl_sched.h>

#define NTASKS 3
#define ONEMILISEC (long long)1000*1000

static pthread_t thread[NTASKS];
long long start_time=0;
static sem_t sem;

static void sig_handler(int sig_rec) {
  unsigned int thread_no;

  thread_no=(unsigned int)(sig_rec-RTL_SIGRTMIN);
  rtl_printf("I'm thread %d, temporally blocked (sem_timedwait) on a semaphore...\n",thread_no);
  rtl_printf("but signals handlers are executed regardless being blocked in a sem\n");
  rtl_printf("After finishing handler execution for signal:%d, I will exit from the semaphore\n",sig_rec);

}

static void *start_routine(void *arg) {
  struct sched_param p;
  struct sigaction sa;
  int signal=0,err=0,j;
  int param=(unsigned) arg;
  rtl_sigset_t mask;

  p . sched_priority =1;//param;
  pthread_setschedparam (pthread_self(), SCHED_FIFO, &p);

  sa.sa_handler=sig_handler;
  sa.sa_mask=0;
  sa.sa_flags=0;

  // Block all signals except the ones you want to receive.
  if (param) {
    rtl_sigfillset(&mask);
    signal=RTL_SIGRTMIN+param;

    rtl_sigdelset(&mask,signal);
    pthread_sigmask(SIG_SETMASK,&mask,NULL);

    rtl_printf("I am thread %d, programing handler for signal:%d\n",param,signal);
    if ((err=sigaction(signal,&sa,NULL))<0 ) {
      rtl_printf("sigaction(%d,&sa,NULL) FAILING, err:%d.\n",signal,err);
    }
  }

  if (!param) {
    sem_wait(&sem);
    usleep(1000);
    rtl_printf("\n\n\n\n -- WAITING FOR THE OTHER TASKS -- \n\n\n");
    rtl_printf("THREAD %d--> I want to signal not odd threads\n",param);

    for (j=1;j<NTASKS;j++) {
      if(thread[j]->magic!=RTL_THREAD_MAGIC)
        rtl_printf("TEST PANIC!!!!!! thread %d not a valid thread\n",j);

      // Send signals only to not odd numbers.
      if (!(j%2))
        pthread_kill(thread[j],RTL_SIGRTMIN+j);
    }

  } else {

    rtl_sigfillset(&mask);
    rtl_sigdelset(&mask,signal);
    pthread_sigmask(SIG_SETMASK,&mask,NULL);

    rtl_printf("THREAD %d. Just before adquiring the sem and getting blocked.\n",param);
    /* Ten seconds */
    err = sem_timedwait (&sem, hrt2ts(clock_gethrtime(CLOCK_REALTIME) + 1000*1000*1000LL * 2));
    rtl_printf("THREAD %d. Something has take me from my timed wait ->errno:%d.\n",param,errno);
    if (!(param % 2)) {
      if (errno == EINTR) {
        rtl_printf("\n\n TEST SUCCESFULLY PASSED!!!! A signal has take thread %d out from the semaphore errno %d \n\n",param,errno);
      } else {
        rtl_printf("\n\n TEST NOT PASSED for thread %d\n\n",param);
      }
    }
  }

  pthread_make_periodic_np(pthread_self(), gethrtime(),1000*1000*1000);
  while(1)
    pthread_wait_np();

  return 0;
}

int init_module(void) {
  int i;

  sem_init(&sem,0,1);

  start_time=gethrtime()+1000*ONEMILISEC;

  // Threads creation.
  for (i=0;i<NTASKS;i++)
    pthread_create (&thread[i], NULL, start_routine,(void *) i);

  return 0;
}

void cleanup_module(void) {
  int i;

  for (i=0;i<NTASKS;i++) {
    pthread_cancel (thread[i]);
    pthread_join (thread[i], NULL);

    //pthread_delete_np (thread[i]);
  }

  sem_destroy(&sem);

}
