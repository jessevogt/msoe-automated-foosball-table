/*
 * POSIX.1 Signals test program
 *
 * Written by J. Vidal
 * Copyright (C) Dec, 2002 OCERA Consortium.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2.
 *
 * Testing nanosleep function when receiving signals.
 *
 * 2003/09/09: ineiti - kicked out start_time and counted backwards
 *                     through the threads, so that the maste-thread
 *                     gets started last.
 */

#include <rtl.h>
#include <rtl_sched.h>

#define NTASKS 3
#define ONEMILISEC (long long)1000*1000
#define ITERS 2


static pthread_t thread[NTASKS];

static void signal_handler(int sig){
  rtl_printf("Signal handler called for signal:%d\n",sig);
}

static void *start_routine(void *arg)
{
  struct sched_param p;
  struct timespec sleep_period,remaining;
  struct sigaction sa;
  rtl_sigset_t mask;
  int signal=0, err=0,i,j=0;
  long long period=0;
  int param=(unsigned) arg;

  //blocking all user signals.
  rtl_sigfillset(&mask);
  pthread_sigmask(SIG_SETMASK,&mask,NULL);  
  
  p . sched_priority =param;
  pthread_setschedparam (pthread_self(), SCHED_FIFO, &p);
  
  sa.sa_handler=signal_handler;
  sa.sa_mask=0;
  sa.sa_flags=0;
  
  signal=RTL_SIGUSR2+param;

  if ((err=sigaction(signal,&sa,NULL))<0 ){
    rtl_printf("sigaction(RTL_SIGRTMIN,&sa,NULL) FAILING, err:%d.\n",err);
  }
  
  sleep_period.tv_sec=1+param;
  remaining.tv_sec=remaining.tv_nsec=sleep_period.tv_nsec=0;

  if (param!=0){
    
    rtl_sigemptyset(&mask);
    rtl_sigaddset(&mask,signal);
    pthread_sigmask(SIG_UNBLOCK,&mask,NULL);

    while (j++<ITERS) {    
      rtl_printf("Hi,I am thread %d\n",param);
      rtl_printf("Sleeping for %ld seconds\n",sleep_period.tv_sec);
      nanosleep(&sleep_period,&remaining);

      if (remaining.tv_sec || remaining.tv_nsec){
	rtl_printf("Warning a signal has wake up thread %d from its sweet dream!\n",param);
	rtl_printf("Still remained %d seconds plus %d nanoseconds\n",remaining.tv_sec,remaining.tv_nsec);
	rtl_printf("Restoring time not sleeped\n");
	sleep_period.tv_sec+=remaining.tv_sec;
	sleep_period.tv_nsec+=remaining.tv_nsec;
      }
    }
  } else {
    period=(long long) ONEMILISEC;
    pthread_make_periodic_np (pthread_self(), gethrtime() ,period );

    while (j++<ITERS){
      pthread_wait_np();
      rtl_printf("Hi, I am thread %d, waking up others threads from its sweet dream\n",param);
      for (i=1;i<NTASKS;i++){
	rtl_printf("Sending signal %d to thread %d\n",RTL_SIGUSR2+i,i);
	err=pthread_kill(thread[i],RTL_SIGUSR2+i);
	rtl_printf("pthread_kill returns %d\n",err);
      }
    }
  }

  return 0;

}

int init_module(void) {
  int i;

  // Threads creation.
  for (i=NTASKS-1;i>=0;i--)
    pthread_create (&thread[i], NULL, start_routine,(void *) i); 
  
  return 0;
}

void cleanup_module(void) {
  int i;
  
  for (i=0;i<NTASKS;i++){
    pthread_cancel(thread[i]);
    pthread_join (thread[i], NULL);
    //pthread_delete(thread[i]);
  }
    
}








