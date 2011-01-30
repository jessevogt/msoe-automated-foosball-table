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
 * Test most of implemented functionalities:
 * pthread_sigmask, sigsuspend, pthread_kill, terminating threads from signal
 * handlers, blocked mask ...
 */

#include <rtl.h>
#include <rtl_sched.h>

#define NTASKS 2
static pthread_t thread[NTASKS];
#define ACK RTL_SIGUSR1
int last_sig_rec=0;

static void signal_handler(int signal){
  rtl_printf("Signal handler called for signal %d\n",signal);
}

static void ack_hdl(int signal){
  static int i=0;
  rtl_printf("Acknolegment %d received\n",i++);
}

static void *start_routine(void *arg)
{
  struct sched_param p;
  struct sigaction sa;
  int err,i=0,j=0;
  rtl_sigset_t set;
  int param=(unsigned) arg;
  
  p . sched_priority =param;
  pthread_setschedparam (pthread_self(), SCHED_FIFO, &p);

  rtl_sigfillset(&set);
  pthread_sigmask(SIG_BLOCK,&set,NULL);

  if (!param){
    sa.sa_handler=ack_hdl;
    if ((err=sigaction(ACK,&sa,NULL))<0 ){
      rtl_printf("sigaction(%d,&sa,NULL) FAILING, err:%d.\n",i,err);
    }
    rtl_sigdelset(&set,ACK);
    j=RTL_SIGUSR1+1;

    while (1) {    
      rtl_printf("Master thread %d waiting acknoledgment before continuing \n",param);
      sigsuspend(&set);
      rtl_printf("Master thread %d sending a window of all signals. Only one is received each time \n",param);
      for (i=RTL_SIGUSR1; i<=j; i++){
	pthread_kill(thread[(param+1)%NTASKS],i);
      }
      j++;
    }
  } else {
    sa.sa_handler=signal_handler;
    rtl_printf("Slave thread %d programing all user signals except RTL_SIGUSR1(overwritting previous programmed)\n",param);
    for (i=RTL_SIGUSR1+1;i<=RTL_SIGRTMAX ;i++){
   
      if ((err=sigaction(i,&sa,NULL))<0 ){
	rtl_printf("sigaction(%d,&sa,NULL) FAILING, err:%d.\n",i,err);
      }

      rtl_printf("Slave thread %d waiting for signal %d \n",param,i);
      pthread_kill(thread[(param+1)%NTASKS],ACK);
      rtl_sigdelset(&set,i);
      sigsuspend(&set);   
      rtl_sigfillset(&set);
      last_sig_rec=i;
      
    }
  } 
  
  return 0;  
} 
 
int init_module(void) {
  int i;
  
  // Threads creation.
  for (i=0;i<NTASKS;i++)
    pthread_create (&thread[i], NULL, start_routine,(void *) i); 
  
  return 0;
}

void cleanup_module(void) {
  int i;

  rtl_printf("\n\n Last signal received by slave %d\n",last_sig_rec);
  for (i=0;i<NTASKS;i++){
    pthread_delete_np (thread[i]);
  }
}






