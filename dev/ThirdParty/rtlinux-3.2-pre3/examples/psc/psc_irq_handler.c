/*
 * Written by Edgar Hilton <efhilton@fsmlabs.com>
 * Copyright (C) Finite State Machine Labs Inc., 1998,1999
 * Released under the terms of the GNU  General Public License Version 2
 */
/*
 * call a user-space allplication signal handler via hardware interrupt 
 */

#include <stdio.h>
#include <rtlinux_signal.h>

#define TEST_IRQ 12         /* from cat /proc/interrupts */

void my_handler(int);
struct rtlinux_sigaction sig, oldsig;

int scount=0;

int main(void) 
{
  sig.sa_handler = my_handler;
  sig.sa_flags = RTLINUX_SA_PERIODIC;
  rtlinux_sigaction( TEST_IRQ, & sig, & oldsig );

  sleep(3);
  
  sig.sa_handler = RTLINUX_SIG_IGN;
  rtlinux_sigaction( TEST_IRQ, & sig, & oldsig );
  
  printf("I got %i mouse interrupts\n",scount);

  /* exit gracefully */
  return 0;
}

void my_handler(int argument)
{
  scount++;
}

