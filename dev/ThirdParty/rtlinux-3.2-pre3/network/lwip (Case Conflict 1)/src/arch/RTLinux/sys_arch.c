/*
 * Copyright (c) 2001, Swedish Institute of Computer Science.
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
 * 3. Neither the name of the Institute nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software 
 *    without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE 
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS 
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
 * SUCH DAMAGE. 
 *
 */

/*********************************************************************************/
/* This file has been written by Sergio Perez Alcañiz <serpeal@disca.upv.es>     */
/*            Departamento de Informática de Sistemas y Computadores             */
/*            Universidad Politécnica de Valencia                                */
/*            Valencia (Spain)                                                   */
/*                                                                               */
/* The RTL-lwIP project has been supported by the Spanish Government Research    */
/* Office (CICYT) under grant TIC2002-04123-C03-03                               */
/*                                                                               */
/* Copyright (c) March, 2003 SISTEMAS DE TIEMPO REAL EMPOTRADOS, FIABLES Y       */
/* DISTRIBUIDOS BASADOS EN COMPONENTES                                           */
/*                                                                               */
/*  This program is free software; you can redistribute it and/or modify         */
/*  it under the terms of the GNU General Public License as published by         */
/*  the Free Software Foundation; either version 2 of the License, or            */
/*  (at your option) any later version.                                          */
/*                                                                               */
/*  This program is distributed in the hope that it will be useful,              */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of               */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                */
/*  GNU General Public License for more details.                                 */
/*                                                                               */
/*  You should have received a copy of the GNU General Public License            */
/*  along with this program; if not, write to the Free Software                  */
/*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA    */
/*                                                                               */
/*  Linking RTL-lwIP statically or dynamically with other modules is making a    */
/*  combined work based on RTL-lwIP.  Thus, the terms and conditions of the GNU  */
/*  General Public License cover the whole combination.                          */
/*                                                                               */
/*  As a special exception, the copyright holders of RTL-lwIP give you           */
/*  permission to link RTL-lwIP with independent modules that communicate with   */
/*  RTL-lwIP solely through the interfaces, regardless of the license terms of   */
/*  these independent modules, and to copy and distribute the resulting combined */
/*  work under terms of your choice, provided that every copy of the combined    */
/*  work is accompanied by a complete copy of the source code of RTL-lwIP (the   */
/*  version of RTL-lwIP used to produce the combined work), being distributed    */
/*  under the terms of the GNU General Public License plus this exception.  An   */
/*  independent module is a module which is not derived from or based on         */
/*  RTL-lwIP.                                                                    */
/*                                                                               */
/*  Note that people who make modified versions of RTL-lwIP are not obligated to */
/*  grant this special exception for their modified versions; it is their choice */
/*  whether to do so.  The GNU General Public License gives permission to        */
/*  release a modified version without this exception; this exception also makes */
/*  it possible to release a modified version which carries forward this         */
/*  exception.                                                                   */
/*********************************************************************************/

#include <rtl_debug.h>
#include <pthread.h>
#include "lwip/sys.h"
#include "lwip/opt.h"
#include "DIDMA.h"
#include "gettimeofday.c"
#include "bcopy.h"
#include <rtl_signal.h>
//#include <rtl_timer.h>
#include <rtl_sync.h>
#include <rtl_sema.h>
#include <rtl.h>
#include <time.h>
#include <signal.h>

//#define THREAD_DEBUG

#define AND(a, b) (a & b)
#define SET(a, b) ((b) <= (0xFF) ? (a=(a | b)) : (a=0x00))

#define MAX_TIMERS 20


extern void rt_free (void *ptr);
extern void *rt_malloc (size_t size);

static struct sys_thread *threads = NULL;

#define MAX_VECTOR_MALLOCS 50

static void *mallocs[MAX_VECTOR_MALLOCS];
static int malloc_index = 0;
static int n_mallocs=0;
static int n_frees = 0;

struct sys_timeouts {
  struct rtl_timer_struct *timer;
  struct sigevent timer_event_spec;
  struct itimerspec ospec;
  struct itimerspec old_setting;
  struct sigaction sa;
  int signal;
};

static struct sys_timeouts vector_of_timers[MAX_TIMERS];
static int timer_index = 0;
static int signal = 0;

struct sys_mbox_msg {
  struct sys_mbox_msg *next;
  void *msg;
};

#define SYS_MBOX_SIZE 100

struct sys_sem {
  sem_t sem;
  unsigned int c;
};

struct sys_mbox {
  u16_t first, last;
  void *msgs[SYS_MBOX_SIZE];
  struct sys_sem *mail;
  struct sys_sem *mutex;
};

struct sys_thread {
  struct sys_thread *next;
  pthread_t pthread;
  char *stack;
  struct sys_timeouts *timeouts;
#ifdef THREAD_DEBUG
  char *name;
#endif /* THREAD_DEBUG */
  char flags;
};

static struct sys_sem *sys_sem_new_(u8_t count);

/*-----------------------------------------------------------------------------------*/
int sys_sem_post(sem_t *sem)
{
  sem_post(sem);
  return 0;
}

/*-----------------------------------------------------------------------------------*/
int sys_sem_signal_pre(struct sys_sem *sem)
{
  rtl_irqstate_t flags;

  sem->c++;
  if(sem->c > 1){
    sem->c = 1;
  }
  sem->sem.value = sem->c;

  // I need this implementation because it doesn't call to rtl_schedule
  rtl_spin_lock_irqsave (&sem->sem.lock, flags);

  ++(sem->sem.value);
  rtl_wait_wakeup (&sem->sem.wait);
  
  rtl_spin_unlock_irqrestore(&sem->sem.lock, flags);

  return 0;
}

/*-----------------------------------------------------------------------------------*/
void sys_stop_interrupts(unsigned int *state){
  rtl_no_interrupts((rtl_irqstate_t) *state);
}

/*-----------------------------------------------------------------------------------*/
void sys_allow_interrupts(unsigned int *state){
    rtl_restore_interrupts((rtl_irqstate_t) *state);
}

/*-----------------------------------------------------------------------------------*/
int obtain_index_to_free(void *mem){
  int i;

  for(i=0; i< MAX_VECTOR_MALLOCS; i++)
    if(mallocs[i] == mem)
      return i;
  return -1;
}

/*-----------------------------------------------------------------------------------*/
void free_all_resources(void){
  int i;

  for(i=0; i<MAX_VECTOR_MALLOCS; i++)
    if(mallocs[i] != NULL){
      rt_free(mallocs[i]);
      n_frees++;
    }
}

/*-----------------------------------------------------------------------------------*/
void *sys_malloc(size_t size){
  void *tmp;
  int entry = malloc_index % MAX_VECTOR_MALLOCS;
  unsigned int state;

  sys_stop_interrupts(&state);

  while(mallocs[entry] != NULL)
    entry = ++malloc_index % MAX_VECTOR_MALLOCS;

  tmp = mallocs[entry] = rt_malloc(size);

  if(tmp != NULL){
    n_mallocs++;
    malloc_index++;
  }else
    rtl_printf("\n\n\n\nERROR: Not enough memory!\n\n\n\n");

  sys_allow_interrupts(&state);

  return tmp;
}

/*-----------------------------------------------------------------------------------*/
void sys_free(void *ptr){
  int index;
  unsigned int state;

  sys_stop_interrupts(&state);

  if(ptr != NULL){
    index = obtain_index_to_free(ptr);
    if(index != -1){
      mallocs[index] = NULL;
      rt_free(ptr);
      n_frees++;
    }else{
      rtl_printf("sys_free: no memory reserved for this pointer\n");
    }
  }

  sys_allow_interrupts(&state);
}

/*-----------------------------------------------------------------------------------*/
static struct sys_thread *
sys_current_thread(void)
{
  struct sys_thread *st;
  pthread_t pt;

  pt = pthread_self();

  for(st = threads; st != NULL; st = st->next) {

    /* If the thread represented by st has exited and it's memory hasn't been deallocated ... */
    if(AND(st->flags,0x81)== 0x80){
      sys_free(st->stack);
      SET(st->flags, 0x81);
      continue;
    }

    if(pthread_equal(st->pthread, pt)) 
      return st;
  }

  return NULL;
}

/*-----------------------------------------------------------------------------------*/
static struct sys_thread *
sys_search_thread(void *pthread)
{
  struct sys_thread *st;
  pthread_t pt = (pthread_t) pthread;

  for(st = threads; st != NULL; st = st->next) {

    /* If the thread represented by st has exited and it's memory hasn't been deallocated ... */
    if(AND(st->flags,0x81)== 0x80){
      sys_free(st->stack);
      SET(st->flags, 0x81);
      continue;
    }

    if(pthread_equal(st->pthread, pt)) 
      return st;
  }

  return NULL;
}

/*-----------------------------------------------------------------------------------*/
#ifdef THREAD_DEBUG
char *thread_name(void)
{
  struct sys_thread *sys_thread;

  sys_thread = sys_current_thread();

  return (char *) sys_thread->name;
}
#endif

/*-----------------------------------------------------------------------------------*/
struct thread_start_param {
  struct sys_thread *thread;
  void (* function)(void *);
  void *arg;
};

/*-----------------------------------------------------------------------------------*/
static void *
thread_start(void *arg)
{
  struct thread_start_param *tp = arg;

  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

  tp->thread->pthread = pthread_self();
  tp->function(tp->arg);

  sys_free(tp);

  return NULL;
}

/*-----------------------------------------------------------------------------------*/
void *sys_thread_exit(void)
{
  struct sys_thread *thread;
  void *status = NULL;
  
  thread = sys_current_thread();

  if(thread->stack != NULL) //i.e. It is a standalone thread
    SET(thread->flags, 0x80);
  else SET(thread->flags, 0x81);

  pthread_exit(status);
  return NULL;
}

/*-----------------------------------------------------------------------------------*/
int sys_thread_delete(void *pthread)
{
  struct sys_thread *thread;
  
  thread = sys_search_thread(pthread);

  if(thread != NULL){
    if(thread->stack != NULL) //i.e. It is a standalone thread
      SET(thread->flags, 0x80);
    else SET(thread->flags, 0x81);
    pthread_cancel(pthread);
    pthread_join(pthread,NULL);
    return 0;
  }


  return -1;
}

/*-----------------------------------------------------------------------------------*/
void sys_thread_register(void *pthread)
{
  struct sys_thread *thread;

  thread = sys_malloc(sizeof(struct sys_thread));


  thread->next = threads;
  thread->pthread = (pthread_t) pthread;
  thread->stack = NULL;
  thread->timeouts = NULL;

  SET(thread->flags, 0x01);
  threads = thread;

  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

}


/*-----------------------------------------------------------------------------------*/
void *
#ifdef THREAD_DEBUG
sys_thread_new(void (* function)(void *arg), void *arg, unsigned long period, char *name, int name_len)
#else
     sys_thread_new(void (* function)(void *arg), void *arg, unsigned long period)
#endif
{
  struct sys_thread *thread;
  struct thread_start_param *thread_param;
  pthread_attr_t attr;

  thread = sys_malloc(sizeof(struct sys_thread));

  if(thread != NULL){

    thread->next = threads;

    SET(thread->flags, 0x00);

    thread->stack = (char *) sys_malloc(sizeof(char)*THREAD_STACK_SIZE);

    thread->timeouts = NULL;

#ifdef THREAD_DEBUG
    thread->name = (char *) sys_malloc(20);
    bcopy(name, thread->name, name_len);
#endif

    if(thread->stack == NULL){
      sys_free(thread);
      rtl_printf("ERROR: Not enough memory to create new thread's stack\n");
      return NULL;
    }

    threads = thread;

    pthread_attr_init(&attr);


    /* Because it's a thread which is trying to create another thread, RTLinux
       only allows that by passing to the new thread it's stack */    
    pthread_attr_setstackaddr(&attr, thread->stack);
    pthread_attr_setstacksize(&attr, THREAD_STACK_SIZE);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    
    thread_param = sys_malloc(sizeof(struct thread_start_param));
    
    if(thread_param == NULL){
      sys_free(thread->stack);
      sys_free(thread);
      rtl_printf("ERROR: Not enough memory to create thread start param\n");
      return NULL;
    }
    
    thread_param->function = function;
    thread_param->arg = arg;
    thread_param->thread = thread;
    
    if(pthread_create(&(thread->pthread),&attr, thread_start, thread_param)) {
      rtl_printf("\nsys_thread_new: pthread_create  0x%x 0x%x\n", pthread_self(),thread->pthread);
      rtl_printf("Kernel Panic\n");
      return NULL;
    }

    if(period != 0)
      pthread_make_periodic_np(thread->pthread, gethrtime(), period);

    return (void *) thread->pthread;
  }
  
  rtl_printf("ERROR: Not enough memory to create thread\n"); 
  return NULL;
}

/*-----------------------------------------------------------------------------------*/
struct sys_mbox *
sys_mbox_new()
{
  struct sys_mbox *mbox;

  mbox = sys_malloc(sizeof(struct sys_mbox));

  if(mbox != NULL){
    
    mbox->first = mbox->last = 0;
    mbox->mail = sys_sem_new_(0);
    mbox->mutex = sys_sem_new_(1);

    return mbox;
  }else{
    rtl_printf("ERROR: Not enough memory to create mbox\n");
    return NULL;
  }
}

/*-----------------------------------------------------------------------------------*/
void
sys_mbox_free(struct sys_mbox *mbox)
{

  if(mbox != SYS_MBOX_NULL) {

    sys_sem_wait(mbox->mutex);
    
    sys_sem_free(mbox->mail);
    sys_sem_free(mbox->mutex);
    mbox->mail = mbox->mutex = NULL;

    sys_free(mbox);
  }
}
/*-----------------------------------------------------------------------------------*/
void
sys_mbox_post(struct sys_mbox *mbox, void *msg)
{
  u8_t first;
  unsigned int state;

  sys_sem_wait(mbox->mutex);
  
  mbox->msgs[mbox->last] = msg;

  if(mbox->last == mbox->first) {
    first = 1;
  } else {
    first = 0;
  }
  
  mbox->last++;
  if(mbox->last == SYS_MBOX_SIZE) {
    mbox->last = 0;
  }

  sys_stop_interrupts(&state);
  
  if(first)
    sys_sem_signal_pre(mbox->mail);

  sys_sem_signal(mbox->mutex);
  
  sys_allow_interrupts(&state);

}
/*-----------------------------------------------------------------------------------*/
u16_t
sys_arch_mbox_fetch(struct sys_mbox *mbox, void **msg, u16_t timeout)
{
  u16_t time = 1;
  
  /* The mutex lock is quick so we don't bother with the timeout
     stuff here. */
  sys_arch_sem_wait(mbox->mutex, 0);
  while(mbox->first == mbox->last) {
    sys_sem_signal(mbox->mutex);
    /* We block while waiting for a mail to arrive in the mailbox. We
       must be prepared to timeout. */
    if(timeout != 0) {

      time = sys_arch_sem_wait(mbox->mail, timeout);
      
      /* If time == 0, the sem_wait timed out, and we return 0. */
      if(time == 0) {
	return 0;
      }
    } else
      sys_arch_sem_wait(mbox->mail, 0);

    sys_arch_sem_wait(mbox->mutex, 0);
  }
  
  if(msg != NULL) {
    *msg = mbox->msgs[mbox->first];
  }
  
  mbox->first++;
  if(mbox->first == SYS_MBOX_SIZE) {
    mbox->first = 0;
  }    
  
  sys_sem_signal(mbox->mutex);
  
  return time;
}

/*-----------------------------------------------------------------------------------*/
struct sys_sem *
sys_sem_new(u8_t count)
{
  return sys_sem_new_(count);
}

/*-----------------------------------------------------------------------------------*/
static struct sys_sem *
sys_sem_new_(u8_t count)
{
  struct sys_sem *sem;

  sem = sys_malloc(sizeof(struct sys_sem));

  if(sem != NULL){
    sem->c = count;
    sem_init(&(sem->sem),0,count);
    return sem;
  }else{
    rtl_printf("ERROR: Not enough memory to create semaphore\n");
    return NULL;
  }
}

/*-----------------------------------------------------------------------------------*/
static u16_t wait_for_semaphore(struct sys_sem *sem, u16_t timeout){
  unsigned int tdiff;
  unsigned long sec, usec;
  struct timeval rtime1, rtime2;
  struct timespec ts;
  struct timezone tz;
  int retval;

  if(timeout > 0) {
    /* Get a timestamp and add the timeout value. */
    gettimeofday(&rtime1, &tz);

    sec = rtime1.tv_sec;
    usec = rtime1.tv_usec;
    usec += timeout % 1000 * 1000;  
    sec += (int)(timeout / 1000) + (int)(usec / 1000000);
    usec = usec % 1000000;
    ts.tv_nsec = usec * 1000;
    ts.tv_sec = sec;
 
    retval = sem_timedwait(&(sem->sem),&ts);   

    if(retval == -1) {
      return 0;
    } else {
      /* Calculate for how long we waited for the cond. */
      gettimeofday(&rtime2, &tz);
      tdiff = (rtime2.tv_sec - rtime1.tv_sec) * 1000 +	
	(rtime2.tv_usec - rtime1.tv_usec) / 1000;
      if(tdiff == 0) {
	return 1;
      }
      return tdiff;
    }
  } else {
    sem_wait(&(sem->sem));
    return 0;
  }
}

/*-----------------------------------------------------------------------------------*/
u16_t
sys_arch_sem_wait(struct sys_sem *sem, u16_t timeout)
{
  u16_t time = 1;

  sem->sem.value = sem->c;

  while(sem->c <= 0) {
    
    if(timeout > 0) {

      time = wait_for_semaphore(sem, timeout);
      if(time == 0) 
	return 0;
    } else {
      wait_for_semaphore(sem,0);
    }
  }

  sem->c--;
  sem->sem.value = sem->c;

  return time;
}

/*-----------------------------------------------------------------------------------*/
int
sys_sem_signal(struct sys_sem *sem)
{

  sem->c++;
  if(sem->c > 1)
    sem->c = 1;

  sem->sem.value = sem->c;

  return sys_sem_post(&(sem->sem));
}

/*-----------------------------------------------------------------------------------*/
void
sys_sem_free(struct sys_sem *sem)
{
  if(sem != NULL)
    sem_destroy(&(sem->sem));

  sys_free(sem);
}

/*-----------------------------------------------------------------------------------*/
void sys_arch_close(void)
{
  struct sys_thread *st;

  for(st = threads; st != NULL; st = st->next) {
    if(AND(st->flags,0x81)== 0x80){ //i.e. It is a standalone thread finished
      sys_free(st->stack);
    }else if(AND(st->flags,0xff)==0x00){ //i.e A thread still working
      pthread_delete_np(st->pthread);
      //This line is necessary because the thread could be standalone.
      //If it is not, nothing happens
      sys_free(st->stack);               
    }else if(AND(st->flags,0xff)==0x01){ //i.e A registered thread still working
      pthread_cancel(st->pthread);
      pthread_join(st->pthread,NULL);
    }
  }

  free_all_resources();

  return;
}

/*-----------------------------------------------------------------------------------*/
void
sys_init(void)
{
  int i,retval;

  for(i=0; i<=500; i++)
    mallocs[i] = NULL;

  for(i=0; i<MAX_TIMERS; i++){
    vector_of_timers[i].signal = RTL_SIGRTMIN + signal++;
    vector_of_timers[i].timer_event_spec.sigev_notify = SIGEV_SIGNAL;
    vector_of_timers[i].timer_event_spec.sigev_signo = vector_of_timers[i].signal;
    vector_of_timers[i].timer_event_spec.sigev_value.sival_int = 13;
    
    retval = timer_create(CLOCK_REALTIME, &(vector_of_timers[i].timer_event_spec), &(vector_of_timers[i].timer));
    
    if (retval) {
      rtl_printf("timer_create(CLOCK_REALTIME) failed:\n");
    }
  }
}

/*-----------------------------------------------------------------------------------*/
void
sys_sem_wait(sys_sem_t sem)
{
  sys_arch_sem_wait(sem, 0);
}

/*-----------------------------------------------------------------------------------*/
int sys_sem_wait_timeout(sys_sem_t sem, u32_t timeout)
{
  return sys_arch_sem_wait(sem, timeout);
}

/*-----------------------------------------------------------------------------------*/
void
sys_mbox_fetch(sys_mbox_t mbox, void **msg)
{
  sys_arch_mbox_fetch(mbox, msg, 0);
}

/*-----------------------------------------------------------------------------------*/
void sys_timeout(u16_t msecs, sys_timeout_handler h, void *arg)
{
  struct sys_thread *thread;
  int err, sec = 0, nsec = 0;

  thread = sys_current_thread();

  if(thread->timeouts == NULL){
    thread->timeouts = &vector_of_timers[timer_index++];
  }

  thread->timeouts->sa.sa_handler = h;
  thread->timeouts->sa.sa_mask=0;
  thread->timeouts->sa.sa_flags=0;
  thread->timeouts->sa.sa_focus=0;

  rtl_sigemptyset(&(thread->timeouts->sa.sa_mask));

  if (sigaction(thread->timeouts->signal, &(thread->timeouts->sa), NULL)) {
    rtl_printf("sigaction failed");
  }  

  if(msecs >= 100){
    sec = msecs / 1000;
    nsec = (msecs % 1000) * 1000000 ;
  }else{
    nsec = msecs * 1000000;
  }

  thread->timeouts->ospec.it_value.tv_sec = sec;
  thread->timeouts->ospec.it_value.tv_nsec = nsec;
  thread->timeouts->ospec.it_interval.tv_sec = 0;
  thread->timeouts->ospec.it_interval.tv_nsec = 0;

  err = timer_settime(thread->timeouts->timer, 0, &(thread->timeouts->ospec), &(thread->timeouts->old_setting));

  return;  
}

/*-----------------------------------------------------------------------------------*/
void sys_untimeout(sys_timeout_handler h, void *arg){
  struct sys_thread *thread;

  thread = sys_current_thread();

  if(thread->timeouts != NULL)
    timer_delete(thread->timeouts->timer);
}
