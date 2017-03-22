/*****************************************************************************
                The Open University - OS course

   File:        binsem.c

   Written by:  yuval idelman 066372368

   Description: this file defines a simple binary semaphores library for
                user-level threads.
                Only 2 values are allowed for a binary semaphore - 0 and 1.
                If a semaphore value is 0, down() on this semaphore will cause
                the calling thread to wait until some other thread raises it
                (by performing up()). Note that any number of therads may be
                waiting on the same semaphore, and up() will allow only one of
                them to continue execution.
                If a semaphore value is 1, up() on this semaphore has no
                effect.


 ****************************************************************************/
#include "binsem.h"
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

/*****************************************************************************
  Initializes a binary semaphore.
  Parameters:
    s - pointer to the semaphore to be initialized.
    init_val - the semaphore initial value. If this parameter is 0, the
    semaphore initial value will be 0, otherwise it will be 1.
*****************************************************************************/
void binsem_init(sem_t *s, int init_val){
	*s = ( init_val == 0 ) ? 0 : 1;
}

/*****************************************************************************
  The Up() operation.
  Parameters:
    s - pointer to the semaphore to be raised.
*****************************************************************************/
void binsem_up(sem_t *s){
	xchg(s,1);
}


/*****************************************************************************
  The Down() operation.
  Parameters:
    s - pointer to the semaphore to be decremented. If the semaphore value is
    0, the calling thread will wait until the semaphore is raised by another
    thread.
  Returns:
      0 - on sucess.
     -1 - on a syscall failure.
*****************************************************************************/
int binsem_down(sem_t *s){
	int semValOld;
	    do {
	        semValOld = xchg(s, 0);
	        if ( semValOld == 0 )
	            if ( kill(getpid(), SIGALRM) == -1 ) {
	                perror("binsem_down failed");
	                return -1;
	            }
	    } while ( semValOld != 1 );
	    return 0;
}















