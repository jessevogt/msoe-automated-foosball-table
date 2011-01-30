/*
 *  Test case for assertion #2 of the sigaction system call that shows
 * sigaction (when used with a non-null oact pointer) changes the action
 * for a signal.
 *
 * Steps:
 * 1. Call sigaction to set handler for SIGUSR1 to use handler1
 * 2. Call sigaction again to set handler for SIGUSR1 to use handler2,
 *    but this time use a non-null oarg and verify the sa_handler for 
 *    oarg is set for handler1.
*/

#include <signal.h>
#include "compat.h"

#define NTASKS 1
static pthread_t thread[NTASKS];


int handler_called = 0;

void handler1(int signo)
{
}

void handler2(int signo)
{
}

void *th_code(void *arg)
{
	struct sigaction act;
	struct sigaction oact;
	
	act.sa_handler = handler1;
	act.sa_flags = 0;
	sigemptyset(&act.sa_mask);
	if (sigaction(SIGUSR1,  &act, 0) == -1) {
		perror("Unexpected error while attempting to setup test "
		       "pre-conditions");
		return NULL;
	}
	
	act.sa_handler = handler2;
	sigemptyset(&act.sa_mask);
	if (sigaction(SIGUSR1,  &act, &oact) == -1) {
		perror("Unexpected error while attempting to setup test "
		       "pre-conditions");
		return NULL;
	}

	if (oact.sa_handler == handler1) {
	  printf("Test PASSED\n");
	  return 0;
	}

	printf("Test Failed\n");
	return NULL;
}


int init_module(void){
  int i;

  for(i=0;i<NTASKS;i++)
    pthread_create(&thread[i], NULL,th_code,(void *) i);

  return 0;  
}

void cleanup_module(void){
 int i;

  for(i=0;i<NTASKS;i++)
    pthread_delete_np(thread[i]);

}






