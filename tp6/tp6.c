#include <pthread.h>
#include <semaphore.h>
#include <syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include "queue.h"

#define mainSleepTie 20  
#define numProd  10
#define numCons  10
#define BUFFER_SIZE 5
#define RAND_DIVISOR 300000000
#define TRUE 1

typedef struct filaint_t
{
   struct filaint_t *prev ;  
   struct filaint_t *next ;  
   int    item ;
} filaint_t ;


filaint_t *fila_teste;
filaint_t *novo,*velho;
pthread_mutex_t mutex;
sem_t s_buffer, s_vaga; 


int counter;
pthread_t tid;       
pthread_attr_t attr; 




void *producer(void *param) {
   int produced=0;
   filaint_t *elem;
   while(TRUE) {
      
      int rNum = random() /RAND_DIVISOR;
      sleep(rNum);
      elem=(filaint_t*)malloc(sizeof(filaint_t));
      elem->item = random()%1000;
      elem->prev = NULL;
      elem->next = NULL;

      sem_wait(&s_vaga); 
      pthread_mutex_lock(&mutex);
		queue_append((queue_t **)&fila_teste,(queue_t *)elem); 
		produced++;
      counter = queue_size((queue_t*) fila_teste);
      printf("Producer %06ld | item %03d \n",
											syscall(SYS_gettid),elem->item);
      
      pthread_mutex_unlock(&mutex);
      
      sem_post(&s_buffer);
   }
}

void *consumer(void *param) {
	int consumed=0;
   filaint_t *velho;
   while(TRUE) {
      
      int rNum = random() / RAND_DIVISOR;
      sleep(rNum);
      
      sem_wait(&s_buffer);
      pthread_mutex_lock(&mutex);
		velho = (filaint_t*)queue_remove ((queue_t**) &fila_teste, (queue_t*) fila_teste);

		consumed++;
      counter = queue_size((queue_t*) fila_teste);
      printf("                   Consumer %06ld | item %03d \n",
											 syscall(SYS_gettid), velho->item);
      free(velho);
      pthread_mutex_unlock(&mutex);
      
      sem_post(&s_vaga);
   }
}

void sjf(){
//    se há uma tarefa rodando
   if ( processo_corrente != NULL ){
//      se a tarefa rodando chegou ao fim da execução
      if (processo_corrente->duracao == processo_corrente->tempo_executado_total) {    
            //migra a tarefa para o estado terminado 
         processo_corrente->estado_atual = 3; // terminado
         queue_append((queue_t **) &finesh, (queue_t *) processo_corrente);
            //libera o processador
         processo_corrente = NULL;
         qtd_Eprocess++;
         pthread_mutex_unlock(&processador);
//      senão
      }else{
//        se a tarefa rodando chegou ao fim de seu quantum
//        migra a tarefa para a fila de prontos
//        libera o processador
//      fim se
      }
//    fim se
   }
   if ((queue_size((queue_t *) finesh)) < (qtd_Tprocess)){
//    para cada tarefa i
      int i;
      iterador = processes;
      for(i = 0; i < queue_size((queue_t *) processes); i++){
//      se a tarefa i inicia agora (em t)
         if( iterador->inicio == tempo){
//        coloca a tarefa na fila de prontos
            aux = (queue_process_t *) queue_remove((queue_t **) &processes, (queue_t *) iterador);
            queue_append((queue_t **) &ready, (queue_t *) iterador);
            iterador->estado_atual = 1;
            iterador = processes;
//      fim se 
         }else{
            iterador = iterador->next;
         }
//    fim para
      }
//    se o processador estiver livre
      if (!pthread_mutex_trylock(&processador)) {
//      se houver tarefa na fila de prontas
         if (queue_size((queue_t *) ready) > 0) {
//        escolhe uma tarefa da fila de prontas
            aux = ready;
            iterador = ready;
            for(i = 0; i < queue_size((queue_t *) ready); i++){
               if (aux->duracao > iterador->duracao){
                  aux = iterador;
                  iterador = iterador->next;
               }else iterador = iterador->next;
            }
//        migra essa tarefa para o estado "rodando"
            processo_corrente = (queue_process_t *) queue_remove((queue_t **) &ready, (queue_t *) aux);
            processo_corrente->estado_atual = 2;
            numero_troca_contexto++;
//      fim se
         }
//    fim se
      }
//    imprime linha do diagrama com o estado de cada tarefa
      imprime();
   }else{
      tempo_medio_vida = (float)(tempo) / (float)qtd_Tprocess;
      tempo = tmax;
   }
}

int main (int argc, char *argv[]){
   int i;
	srand(time(NULL));
  
   pthread_mutex_init(&mutex, NULL);
   sem_init(&s_buffer, 0, 0);
   sem_init(&s_vaga, 0, BUFFER_SIZE);
   pthread_attr_init(&attr);
   counter = 0;

   pthread_t thread [numProd+numCons];
   for(i = 0; i < numProd; i++) {
     
      thread[i]=pthread_create(&tid,&attr,producer,NULL);
    }

   
   for(i = 0; i < numCons; i++) {
      thread[numProd+i]=pthread_create(&tid,&attr,consumer,NULL);
   }

	pthread_exit (NULL) ;

   printf("Exit the program\n");
   exit(0);
}

