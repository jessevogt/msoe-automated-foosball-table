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
 * sys.h
 *                     
 * Author : Adam Dunkels <adam@sics.se>                               
 *
 * CHANGELOG: this file has been modified by Sergio Perez Alca�iz <serpeal@disca.upv.es> 
 *            Departamento de Inform�tica de Sistemas y Computadores          
 *            Universidad Polit�cnica de Valencia                             
 *            Valencia (Spain)    
 *            Date: March 2003                                          
 * 
 */

#ifndef __RTL_LWIP_SYS_H__
#define __RTL_LWIP_SYS_H__

#include "arch/cc.h"
#include "arch/sys_arch.h"

typedef void (* sys_timeout_handler)(int signo);

/* sys_init() must be called before anthing else. */
void sys_init(void);

/*
 * sys_timeout():
 *
 * Schedule a timeout a specified amount of milliseconds in the
 * future. When the timeout occurs, the specified timeout handler will
 * be called. The handler will be passed the "arg" argument when
 * called.
 *
 */
void sys_timeout(u16_t msecs, sys_timeout_handler h, void *arg);
struct sys_timeouts *sys_arch_timeouts(void);
void sys_untimeout(sys_timeout_handler h, void *arg);

/* Semaphore functions. */
sys_sem_t sys_sem_new(u8_t count);
int sys_sem_signal(sys_sem_t sem);
u16_t sys_arch_sem_wait(sys_sem_t sem, u16_t timeout);
void sys_sem_free(sys_sem_t sem);

void sys_sem_wait(sys_sem_t sem);

int sys_sem_wait_timeout(sys_sem_t sem, u32_t timeout);

/* Mailbox functions. */
sys_mbox_t sys_mbox_new(void);
void sys_mbox_post(sys_mbox_t mbox, void *msg);
u16_t sys_arch_mbox_fetch(sys_mbox_t mbox, void **msg, u16_t timeout);
void sys_mbox_free(sys_mbox_t mbox);

void sys_mbox_fetch(sys_mbox_t mbox, void **msg);

/* Thread functions. */
//void sys_thread_new(void (* thread)(void *arg), void *arg, unsigned long period, char *name, int name_len);
void *sys_thread_new(void (* function)(void *arg), void *arg, unsigned long period);
/* The following functions are used only in Unix code, and
   can be omitted when porting the stack. */
/* Returns the current time in microseconds. */
void sys_arch_close(void);
void * sys_thread_exit(void);
int sys_thread_delete(void *pthread);
void sys_thread_register(void *pthread);
void register_tcpip_thread(void);
void *sys_malloc(size_t size);
void sys_free(void *ptr);
void sys_stop_interrupts(unsigned int *state);
void sys_allow_interrupts(unsigned int *state);


extern void *rt_realloc(void *p, size_t new_len);
/*
#define memp_malloc(x) (sys_malloc(memp_sizes[x]))
#define memp_malloc2(x) (sys_malloc(memp_sizes[x]))
#define memp_mallocp(x) (sys_malloc(memp_sizes[x]))
#define memp_realloc(x,y,z) (rt_realloc(z,memp_sizes[y]))
#define memp_free(x,y) (sys_free(y))
#define memp_freep(x,y) (sys_free(y))

#define mem_malloc(x) (sys_malloc(x))
#define mem_free(x) (sys_free(x))
#define mem_realloc(x,y) (sys_realloc(x,y))
*/

#endif /* __RTL_LWIP_SYS_H__ */
