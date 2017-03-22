/*****************************************************************************
                The Open University - OS course

   File:        ut.c

   Written by:  Yuval Idelman 066372368

   Description:  library for creating & scheduling
                user-level threads.


 ****************************************************************************/
#include "ut.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <sys/time.h>
#include <unistd.h>
static ut_slot_t* ttable;
static ucontext_t uc_Main;
static int threads_count=0;
static volatile int currThreadNum=0;
static int tabSize;
static struct sigaction sa;

/*handler to handle the signals and swap between the threds*/
void handler(int signal){
	if(signal==SIGALRM)
	 {
		struct itimerval itv;
		int prevThread = currThreadNum;
		if ( getitimer( ITIMER_VIRTUAL, &itv ) == -1 ) {
					perror( "getitimer failed: " );
					exit( 1 );
				}
		ttable[currThreadNum].vtime +=itv.it_interval.tv_usec - itv.it_value.tv_usec;
		itv.it_value = itv.it_interval;
				if ( setitimer( ITIMER_VIRTUAL, &itv, NULL ) == -1 ) {
					perror( "setitimer failed: " );
					exit( 1 );
				}
		alarm(1);
		currThreadNum =(currThreadNum+1) % threads_count;
		//printf("in signal handler: switching from %d to %d\n", prevThread, currThreadNum);
		if ( swapcontext( &(ttable[prevThread].uc ),&( ttable[currThreadNum].uc ) ) == -1 ) {
		            perror("swap  failed:");
		            exit( 1 );
		        }
	}
	ttable[currThreadNum].vtime += 100000;
}


  /*****************************************************************************
   Initialize the library data structures and the sigaction handler. Create the threads table. If the given
   size is outside the range [MIN_TAB_SIZE,MAX_TAB_SIZE], the table size will be
   MAX_TAB_SIZE.

   Parameters:
      tab_size - the threads_table_size.

   Returns:
      0 - on success.
  	SYS_ERR - on table allocation failure.
  *****************************************************************************/
int ut_init(int tab_size){
	sa.sa_flags = SA_RESTART;
	sigfillset( &sa.sa_mask );
	sa.sa_handler = handler;

	if ( sigaction( SIGALRM, &sa, NULL ) < 0 )
		 return SYS_ERR;

	if(tab_size>MAX_TAB_SIZE||tab_size<MIN_TAB_SIZE)
	{
		tab_size=MAX_TAB_SIZE;
	}
	tabSize=tab_size;
	ttable=(ut_slot_t*)malloc(tab_size * sizeof(ut_slot_t));
	if(ttable==NULL)
	{
		return SYS_ERR;
	}
	return 0;
}


/*****************************************************************************
 Add a new thread to the threads table. Allocate the thread stack and update the
 thread context accordingly. This function DOES NOT cause the new thread to run.
 All threads start running only after ut_start() is called.

 Parameters:
    func - a function to run in the new thread. We assume that the function is
	infinite and gets a single int argument.
	arg - the argument for func.

 Returns:
	non-negative TID of the new thread - on success (the TID is the thread's slot
	                                     number.
    SYS_ERR - on system failure (like failure to allocate the stack).
    TAB_FULL - if the threads table is already full.
 ****************************************************************************/
tid_t ut_spawn_thread(void (*func)(int), int arg){
	if(threads_count==tabSize)
	{
		return TAB_FULL;
	}
	ttable[threads_count].vtime=0;
	ttable[threads_count].arg=arg;
	ttable[threads_count].func=(*func);
	ttable[threads_count].uc.uc_link=&uc_Main;
	ttable[threads_count].uc.uc_stack.ss_sp=malloc(STACKSIZE);
	if(ttable[threads_count].uc.uc_stack.ss_sp==NULL)
	{
		return SYS_ERR;
	}
	ttable[threads_count].uc.uc_stack.ss_size=STACKSIZE;
	if (getcontext( &ttable[threads_count].uc ) == -1)
				return SYS_ERR;
	return threads_count++;
}

/*****************************************************************************
 Starts running the threads, previously created by ut_spawn_thread. Sets the
 scheduler to switch between threads every second (this is done by registering
 the scheduler function as a signal handler for SIGALRM, and causing SIGALRM to
 arrive every second). Also starts the timer used to collect the threads CPU usage
 statistics and establishes an appropriate handler for SIGVTALRM,issued by the
 timer.
 The first thread to run is the thread with TID 0.

 Parameters:
    None.

 Returns:
    SYS_ERR - on system failure (like failure to establish a signal handler).
    Under normal operation, this function should start executing threads and
	never return.
 ****************************************************************************/
int ut_start(void){
	struct itimerval itv;
	int i;
	currThreadNum = 0;
	itv.it_interval.tv_sec = 0;
	itv.it_interval.tv_usec = 10000;
	itv.it_value = itv.it_interval;
	if (setitimer( ITIMER_VIRTUAL, &itv, NULL ) < 0 )
			return SYS_ERR;

	if(sigaction(SIGALRM, &sa, NULL) < 0 ){
		    return SYS_ERR;
		}
	for(i=0;i!=threads_count;i++){
		makecontext(&ttable[i].uc, (void(*)(void))ttable[i].func, 1,ttable[i].arg);
	}
		alarm(1);
		if ( swapcontext( &uc_Main, &( ttable[0].uc ) ) == -1 )
			return SYS_ERR;
		return 0;
}

/*****************************************************************************
 Returns the CPU-time consumed by the given thread.

 Parameters:
    tid - a thread ID.

 Returns:
	the thread CPU-time (in millicseconds).
 ****************************************************************************/
unsigned long ut_get_vtime(tid_t tid){
	return ttable[tid].vtime/1000;
}


