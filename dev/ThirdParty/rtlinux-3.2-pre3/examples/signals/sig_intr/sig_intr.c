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
 * Testing signal handlers preemption.
 *
 */

#include <rtl.h>
#include <pthread.h>

#define NTASKS 2
static pthread_t thread[NTASKS];
#define ITERS 100
static int count=0;

static void signal_handler(int signal){
  int i,j,k,m=0;

  rtl_printf("Signal handler called for signal %d, just wasting time\n",signal);
  for (i=0;i<10;i++){
    rtl_printf(" Signal handler computing m:%d\n",m);
    for (j=0;j<ITERS;j++)
      for (k=0;k<ITERS;k++)
	m=i+k-j;
      }

  rtl_printf("Signal handler %d. Just before ending\n",signal);

}

static void *hight_prio_routine(void *arg){
  struct sched_param p;

  p.sched_priority=100;
  pthread_setschedparam (pthread_self(), SCHED_FIFO, &p);
  pthread_make_periodic_np(pthread_self(), gethrtime(), 100*1000LL);

  pthread_kill(thread[0],RTL_SIGUSR1);
  while (count++<ITERS){
    rtl_printf("Interrumping signal handler execution -- iter: %d\n",count);
    pthread_wait_np();
  }
  
 return 0;
}


static void *start_routine(void *arg)
{
  struct sched_param p;
  int param,err;
  struct sigaction sa;
  rtl_sigset_t mask;


  param=(unsigned) arg;
  p . sched_priority = param;
  pthread_setschedparam (pthread_self(), SCHED_FIFO, &p);
  
  sa.sa_handler=signal_handler;
  sa.sa_mask=0;
  sa.sa_flags=0;
  
  if ((err=sigaction(RTL_SIGUSR1,&sa,NULL))<0 ){
    rtl_printf("sigaction(RTL_SIGUSR1,&sa,NULL) FAILING, err:%d.\n",err);
    pthread_exit(NULL);
  }
  
  rtl_sigfillset(&mask);
  rtl_sigdelset(&mask,RTL_SIGUSR1);
  sigsuspend(&mask);

  rtl_printf("TEST SUCCED !!!!!!!!\n");
  count=ITERS+1;
  rtl_printf("terminating hight priority routine \n");
  return 0;
}

int init_module(void) {
  int i;


  for (i=0;i<NTASKS-1;i++)
    pthread_create (&thread[i], NULL, start_routine, (int *) i);

  // This thread will interrupt lower priority thread signal handler execution.
  pthread_create(&thread[NTASKS-1],NULL,hight_prio_routine,(void *)NULL);
  
  return 0;
}

void cleanup_module(void) {
  int i;
  for (i=0;i<NTASKS;i++)
    pthread_delete_np (thread[i]);
}








