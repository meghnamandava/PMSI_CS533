/******************************************************************************
 * * FILE: hello.c
 * * DESCRIPTION:
 * *   A "hello world" Pthreads program.  Demonstrates thread creation and
 * *   termination.
 * * AUTHOR: Blaise Barney
 * * LAST REVISED: 08/09/11
 * ******************************************************************************/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "m5op.h"
#define NUM_THREADS	2

unsigned long int array[16384];
void *compute1(void *threadid)
{	
	long tid;
  tid = (long)threadid;

	unsigned long int i;
	unsigned long int a = 0;
	
	for (i = 0; i<16384; i++) {
		array[i] = a * 7;
		a = i*4;
	}	
  pthread_exit(NULL);
}

void *compute2(void *threadid) 
{
	unsigned long int i;
	unsigned long int b[16384];
	for (i = 0; i<16384; i++) {
		b[i] = array[i] * 9;
	}
	pthread_exit(NULL);
}


int main(int argc, char *argv[])
{
	pthread_t threads[NUM_THREADS];
  int rc;
  long t;

	/*
  for(t=0;t<NUM_THREADS;t++){
	  printf("In main: creating thread %ld\n", t);

	  rc = pthread_create(&threads[t], NULL, PrintHello, (void *)t);
	  if (rc){
	  	printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(-1);
		}
	}
	*/
	m5_work_begin(0,0);
	rc = pthread_create(&threads[0], NULL, compute1, (void*)0);
  if (rc){
	  	printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(-1);
		}
	rc = pthread_create(&threads[1], NULL, compute2, (void*)0);
  if (rc){
	  	printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(-1);
		}
	m5_work_end(0,0);
  /* Last thing that main() should do */
	pthread_exit(NULL);
}
