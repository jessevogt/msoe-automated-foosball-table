#include <rtl.h>
#include <pthread.h>

#define SIGUSR1 RTL_SIGUSR1
#define SIGALARM RTL_SIGRTMAX

#define printf rtl_printf
#define perror rtl_printf
#define sigemptyset rtl_sigemptyset
#define sigset_t rtl_sigset_t
#define exit(why) pthread_exit((void *)why)
#define sleep(s) (int)({usleep((s)*1000*1000);})
extern int raise(int signal) { 
  int err=0;

  err=pthread_kill(pthread_self(),signal);
  rtl_schedule(); 
  return err;
} 

