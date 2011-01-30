#include <rtl.h>
#include <rtl_sched.h>

#undef SIGUSR1
#define SIGUSR1 RTL_SIGUSR1


#define printf rtl_printf
#define perror rtl_printf
#define sigemptyset rtl_sigemptyset

extern int raise(int signal) { 
  int err=0;

  err=pthread_kill(pthread_self(),signal);
  rtl_schedule(); 
  return err;
} 

