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
 * Testing pending and blocked masks of user signals.
 *
 * 2003/09/09: ineiti - kicked out start_time and counted backwards
 *                     through the threads, so that the maste-thread
 *                     gets started last.
 */
#include <rtl.h>
#include <rtl_sched.h>

#define NTASKS 4
#define ONEMILISEC (long long)1000*1000
static pthread_t thread[NTASKS];

static void waste_time(int i){
  int j,k,l,m=0;
  
  // Consume time
  for (j=0;j<(10+10*i);j++)
    for (k=0;k<10;k++)
      for (l=0;l<10;l++)
	m=m+j-k+l;
}

static void send_signals(void){
  int i=0;
  int target_thread_no=0;
  
  rtl_printf("I am the master & I'm just sending signals to the rest of threads.\n");
  // Wake up the rest of threads
  for (i=1 ;i<NTASKS;i++){
    target_thread_no = i;
    rtl_printf("Waking up thread:%d with signal %d.\n",i,RTL_SIGRTMIN+i);
    if (thread[target_thread_no] && 
	thread[target_thread_no]->magic  == RTL_THREAD_MAGIC)
      pthread_kill(thread[target_thread_no],RTL_SIGRTMIN+i);
    else
      rtl_printf("thread:%d not a valid thread.\n",target_thread_no);
  }
}


static void signal_handler(int sig){
  rtl_printf("I am a slave thread executing the handeler for signal:%d \n",sig);
  rtl_printf("Waking up!\n");
  pthread_wakeup_np(pthread_self());
}


static void *start_routine(void *arg)
{
  struct sched_param p;
  struct sigaction sa;
  int signal;
  int err;
  long period;
  static int count=0;
  rtl_sigset_t set,oset;
  int param=(unsigned) arg;
  
  p . sched_priority =param;
  pthread_setschedparam (pthread_self(), SCHED_FIFO, &p);
  
  period=(long)500*ONEMILISEC;
  signal=RTL_SIGRTMIN+param;
 
  if (param){ 
    sa.sa_handler=signal_handler; 
    sa.sa_mask=0;
    sa.sa_flags=0;
    
    if ((err=sigaction(signal,&sa,NULL))<0 ){
      rtl_printf("sigaction(signal,&sa,NULL) FAILING, err:%d.\n",signal,err);
    }
  }
  
  // only one task is periodic. The others are woken up by signals
  if (param == 0) {
    rtl_printf("Only one task is periodic. The others are waken up by signals\n");
    pthread_make_periodic_np (pthread_self(), gethrtime() ,period );
    
    while (1) {    
      send_signals();
      pthread_wait_np();
    }
  } else {
    while (1){
          pthread_suspend_np(pthread_self());
	  rtl_printf("Slave thread nr:%d. Just wasting time\n",param);
	  waste_time(signal);
	  if (count++> signal){
	    rtl_printf("Slave thread tired of reciving signal %d, %d times.\n",signal,signal);
	    rtl_printf("Just blocking it. After this, It will remain suspended forever and ever!\n");
	    rtl_sigemptyset(&set);
	    rtl_sigaddset(&set,signal);
	    pthread_sigmask(SIG_BLOCK, &set,&oset );
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



