/*PROGRAMA RETIRADO DO SITE:
https://macboypro.wordpress.com/2009/05/25/producer-consumer-problem-using-cpthreadsbounded-buffer/

ALGUMAS PARTES DO CÓDIGO FORAM ALTERADAS.
*/

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include "check.h"
#include <syscall.h>

#define RAND_DIVISOR 300000000
#define TRUE 1

typedef int buffer_item;
#define BUFFER_SIZE 5

/* The mutex lock */
pthread_mutex_t mutex;

/* the semaphores */
sem_t full, empty;

/* the buffer */
buffer_item buffer[BUFFER_SIZE];

/* buffer counter */
int counter;

pthread_t tid;       //Thread ID
pthread_attr_t attr; //Set of thread attributes

void *producer(void *param); /* the producer thread */
void *consumer(void *param); /* the consumer thread */

void initializeData() {
   /* Create the mutex lock */
   pthread_mutex_init(&mutex, NULL);
   /* Create the full semaphore and initialize to 0 */
   sem_init(&full, 0, 0);
   /* Create the empty semaphore and initialize to BUFFER_SIZE */
   sem_init(&empty, 0, BUFFER_SIZE);
   /* Get the default attributes */
   pthread_attr_init(&attr);
   /* init buffer */
   counter = 0;
}
/* Producer Thread */
void *producer(void *param) {
	int produced=0;//Amount of items produced
   buffer_item item;
   while(TRUE) {
      /* sleep for a random period of time */
      int rNum = rand() / RAND_DIVISOR;
      sleep(rNum);
      /* generate a random number */
      item = rand()%100;
      /* acquire the empty lock */
      sem_wait(&empty); //Locks a producer in case buffer is full (i.e.empty<=0)
      /* acquire the mutex lock */
      pthread_mutex_lock(&mutex);
		CHECK(insert_item(item)==0); //insert is "protected" by mutex
		produced++;
      printf("Producer %06d | item %03d -- counter: %3d -- produced: %d\n",
											syscall(SYS_gettid), item,counter,produced);
      /* release the mutex lock */
      pthread_mutex_unlock(&mutex);
      /* signal full */
      sem_post(&full);
   }
}
/* Consumer Thread */
void *consumer(void *param) {
	int consumed=0;//Amount of items consumed
   buffer_item item;
   while(TRUE) {
      /* sleep for a random period of time */
      int rNum = rand() / RAND_DIVISOR;
//      sleep(rNum);
      /* aquire the full lock */
		//full -> quantidade de posicoes preenchidas
		//se  full<=0 o consumidor entra em espera.
      sem_wait(&full);//locks consumer in case buffer in empty (full<=0)
      /* aquire the mutex lock */
      pthread_mutex_lock(&mutex);
		CHECK(remove_item(&item)==0); //remove is "protected" by mutex
		consumed++;
      printf("Consumer %06d | item %03d -- counter: %3d -- consumed: %d\n",
											 syscall(SYS_gettid), item,counter,consumed);
      /* release the mutex lock */
      pthread_mutex_unlock(&mutex);
      /* signal empty */
      sem_post(&empty);
   }
}

/* Add an item to the buffer */
int insert_item(buffer_item item) {
   /* When the buffer is not full add the item
      and increment the counter*/
   if(counter < BUFFER_SIZE) {
      buffer[counter] = item;
      counter++;
      return 0;
   }
   else { /* Error the buffer is full */
      return -1;
   }
}

/* Remove an item from the buffer */
int remove_item(buffer_item *item) {
   /* When the buffer is not empty remove the item
      and decrement the counter */
   if(counter > 0) {
      *item = buffer[(counter-1)];
      counter--;
      return 0;
   }
   else { /* Error buffer empty */
      return -1;
   }
}

int main(int argc, char *argv[]) {
   /* Loop counter */
   int i;
	srand(time(NULL));
   /* Verify the correct number of arguments were passed in */
   if(argc != 4) {
      fprintf(stderr, "USAGE:./main.out <INT> <INT> <INT>\n");
   }

   int mainSleepTime = atoi(argv[1]); /* Time in seconds for main to sleep */
   int numProd = atoi(argv[2]); /* Number of producer threads */
   int numCons = atoi(argv[3]); /* Number of consumer threads */

   /* Initialize the app */
   initializeData();

   pthread_t thread [numProd+numCons];
   /* Create the producer threads */
   for(i = 0; i < numProd; i++) {
      /* Create the thread */
      thread[i]=pthread_create(&tid,&attr,producer,NULL);
    }

   /* Create the consumer threads */
   for(i = 0; i < numCons; i++) {
      /* Create the thread */
      thread[numProd+i]=pthread_create(&tid,&attr,consumer,NULL);
   }

   /* Sleep for the specified amount of time in milliseconds */
   sleep(mainSleepTime);

   /* Exit the program */
   printf("Exit the program\n");
   exit(0);
}

