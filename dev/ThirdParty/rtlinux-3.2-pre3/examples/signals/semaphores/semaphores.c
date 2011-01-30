/*
 * Added user signals to semaphores test,
 * Dec, 2002 Josep Vidal <jvidal@disca.upv.es> (OCERA)
 */

#include <rtl.h>
#include <rtl_sema.h>
#include <rtl_sched.h>

#define NTASKS 20
#define ONEMILISEC (long long)1000*1000
#define ITERS 5

static pthread_t thread[NTASKS];
long long start_time=0;
static sem_t sem;
static int th_wakeup[NTASKS];

void sig_handler(int sig_rec) {
  unsigned int thread_no;

  thread_no=(unsigned int)(sig_rec-RTL_SIGRTMIN);
  rtl_printf("I'm thread %d, really blocked on a sem...\n",thread_no);
  rtl_printf("but signals handlers are executed regardless being blocked in a sem\n");
  rtl_printf("After finishing handler execution for signal:%d, I will exit from the semaphore\n",sig_rec);
}

static void *start_routine(void *arg) {
  struct sched_param p;
  struct sigaction sa;
  int signal=0,err=0,j,iter=0;
  long long period=0;
  int param=(unsigned) arg;
  rtl_sigset_t mask;

  p . sched_priority =1;//param;
  pthread_setschedparam (pthread_self(), SCHED_FIFO, &p);


  // Block all signals except the ones you want to receive.

  if (param) {
    rtl_sigfillset(&mask);
    signal=RTL_SIGRTMIN+param;

    rtl_sigdelset(&mask,signal);
    pthread_sigmask(SIG_SETMASK,&mask,NULL);

    rtl_printf("I am thread %d, programing handler for signal:%d\n",param,signal);

    sa.sa_handler=sig_handler;
    sa.sa_mask=0;
    sa.sa_flags=0;


    if ((err=sigaction(signal,&sa,NULL))<0 ) {
      rtl_printf("sigaction(%d,&sa,NULL) FAILING, err:%d.\n",signal,err);
    }
  }

  if (!param) {
    sem_wait(&sem);
    period=500*ONEMILISEC;
    pthread_make_periodic_np (pthread_self(), start_time ,period );
    while (iter++<ITERS) {
      rtl_printf("\n\n\n\n -- MASTER thread in iter:%d BEFORE WAITING FOR NEXT PERIOD -- \n\n\n",iter);
      rtl_printf("THREAD %d--> I want to signal not odd threads\n",param);
      pthread_wait_np();

      for (j=1;j<NTASKS;j++) {
        if(thread[j]->magic!=RTL_THREAD_MAGIC)
          rtl_printf("TEST PANIC!!!!!! thread %d not a valid thread\n",j);

        // Send signals only to not odd numbers.
        if (!(j%2))
          pthread_kill(thread[j],RTL_SIGRTMIN+j);
      }
    }
  } else {
    rtl_sigfillset(&mask);
    rtl_sigdelset(&mask,signal);

    //    while(iter++<ITERS){
    rtl_printf("THREAD %d. Just before adquiring the sem and getting blocked for ever.\n",param);
    err=sem_wait(&sem);
    rtl_printf("thread %d, sem_wait returned:%d & errno:%d  (#define EINTR 4  /* Interrupted system call */) \n",param,err,errno);

    if (!(param % 2))
      rtl_printf("TEST SUCCESFULLY PASSED!!!! A signal has take thread %d out from the semaphore (iter:%d)\n",param,iter);
    else
      rtl_printf("TEST PANIC!!!! thread %d must be blocked on the semaphore (iter:%d)\n",param,iter);

    // }

  }

  rtl_printf("Thread %d just before ending\n",param);
  th_wakeup[param]=1;

  return (void *)35 + param;
}

int init_module(void) {
  int i;

  sem_init(&sem,0,1);

  start_time=gethrtime()+1000*ONEMILISEC;

  // Threads creation.
  for (i=0;i<NTASKS;i++) {
    th_wakeup[i]=0;
    pthread_create (&thread[i], NULL, start_routine,(void *) i);
  }
  return 0;
}

void cleanup_module(void) {
  int i;

  for (i=0;i<NTASKS;i++) {
    if (i && th_wakeup[i])
      rtl_printf("thread %d waken up by a signal.\n",i);
    pthread_cancel (thread[i]);
    pthread_join (thread[i], NULL);

    //pthread_delete_np (thread[i]);
  }

  rtl_printf("I'm printing this because non-periodic threads rtl_printfs are cutted\n");

  sem_destroy(&sem);
}

