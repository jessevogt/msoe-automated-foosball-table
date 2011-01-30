/*
 *  Test case for assertion #1 of the sigaction system call that shows
 * sigaction (when used with a non-null act pointer) changes the action
 * for a signal.
 *
 * Steps:
 * 1. Initialize a global variable to indicate the signal
 *    handler has not been called. (A signal handler of the 
 *    prototype "void func(int signo);" will set the global
 *    variable to indicate otherwise. 
 * 2. Use sigaction to setup a signal handler for SIGUSR1
 * 3. Raise SIGUSR1.
 * 4. Verify the global indicates the signal was called.
*/

#include <signal.h>
#include "compat.h"

#define NTASKS 1
static pthread_t thread[NTASKS];

int handler_called = 0;

void handler(int signo)
{
	handler_called = 1;
}

void * th_code (void * arg)
{
	struct sigaction act;
	
	act.sa_handler = handler;
	act.sa_flags = 0;
	sigemptyset(&act.sa_mask);
	if (sigaction(SIGUSR1,  &act, 0) == -1) {
		perror("Unexpected error while attempting to setup test "
		       "pre-conditions");
		return NULL;
	}
	
	if (raise(SIGUSR1) == -1) {
		perror("Unexpected error while attempting to setup test "
		       "pre-conditions");
		return NULL;
	}

	if (handler_called) {
		printf("Test PASSED\n");
		return 0;
	}

	printf("Test FAILED\n");
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






