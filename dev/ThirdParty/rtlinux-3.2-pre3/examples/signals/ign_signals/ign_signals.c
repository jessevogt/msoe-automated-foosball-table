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
 *  In this test a master sends signals to all its slaves. 
 *  Only odd threads should get delivered generated signals. 
 *  So the others threads have installed the default handler or SIG_IGN.
 *
 * 2003/09/09: ineiti - kicked out start_time and counted backwards
 *                     through the threads, so that the maste-thread
 *                     gets started last.
 */

#include <rtl.h>
#include <rtl_sched.h>

#define NTASKS 8
#define ONEMILISEC (long long)1000*1000
static pthread_t thread[NTASKS];

static void send_signals(void){
  static int count=0;
  int i=0;
  int target_thread_no=0;
  
  rtl_printf("I am the master & I'm just sending signals to the rest of threads iter:%d.\n",
	     count++);

  // Wake up the rest of threads
  for (i=1 ;i<NTASKS;i++){
    target_thread_no = i;
    if (thread[target_thread_no] && 
	thread[target_thread_no]->magic  == RTL_THREAD_MAGIC)
      pthread_kill(thread[target_thread_no],RTL_SIGRTMIN+i);
    else
      rtl_printf("thread:%d not a valid thread.\n",target_thread_no);
  }
}


static void signal_handler(int sig){
  rtl_printf("I am a slave thread number:%d executing the handler for signal:%d \n",
	     sig-RTL_SIGRTMIN,sig);
}

static void *start_routine(void *arg)
{
  struct sched_param p;
  struct sigaction sa;
  int signal;
  int err;
  long period;
  rtl_sigset_t set;
  int param=(unsigned) arg;

  rtl_sigfillset(&set);
  pthread_sigmask(SIG_BLOCK,&set,NULL);
  
  p . sched_priority =param;
  pthread_setschedparam (pthread_self(), SCHED_FIFO, &p);
  
  period=(long)2*100*ONEMILISEC;
  signal=RTL_SIGRTMIN+param;
  
  if (param){ 
    if (param % 2)
      sa.sa_handler=signal_handler; 
    else{
      if (param!=NTASKS-1) 
	sa.sa_handler=SIG_IGN;
      else
	sa.sa_handler=SIG_DFL;

    }

    sa.sa_mask=0;
    sa.sa_flags=0;
    
    if ((err=sigaction(signal,&sa,NULL))<0 ){
      rtl_printf("sigaction(%i,&sa,NULL) FAILING, err:%i.\n",signal,err);
    }
  }
  
  // only one task is periodic. The others are woken up by signals
  if (param == 0) {
    pthread_make_periodic_np (pthread_self(), gethrtime() ,period );
    
    while (1) {    
      pthread_wait_np();
      send_signals();
    }
  } else {

    rtl_sigdelset(&set,signal);
    
    while (1){
      sigsuspend(&set);
      rtl_printf("Slave thread nr:%d. Just wasting time\n",param);
      if (!(param % 2)) {
	rtl_printf("TEST FAILED!!!!!!!!!! An ignored signal has woken me up!!!!!!!!\n");
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

  for (i=0;i<NTASKS;i++)
    pthread_delete_np (thread[i]);
    
}



