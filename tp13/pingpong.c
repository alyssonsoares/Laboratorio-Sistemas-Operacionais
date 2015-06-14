// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DAINF UTFPR
// Versão 1.0 -- Março de 2015
//
// Interface do núcleo para as aplicações

#include <stdlib.h>
#include <stdio.h>
#include "datatypes.h"		// estruturas de dados necessárias
#include <ucontext.h>
#include "queue.h"
#include <sys/time.h>
#include <signal.h>
// #define DEBUG 1
#define TRUE 1
#define FALSE 0

task_t task_main,dispatcher;
task_t *task_corrente;
task_t *ready = NULL;
task_t *next = NULL;
int id;
int alpha= -1;
int quantum;

int time;
int taskStart, taskEnd;

//tratador de sinal
struct sigaction action ;
//timer
struct itimerval timer;
unsigned int systime();

// funções gerais ==============================================================


void ticks(int signum){
	time++;
	if(task_corrente->taskUser){
		if(quantum == 0){
			taskEnd=systime();
			task_corrente->tempoExec += taskEnd - taskStart;
			task_corrente->ativacoes++;
			taskEnd = 0;
			taskStart = 0;
			task_yield();
		}
		else{
			quantum--;
		}
	}else{
		taskEnd = systime();
		task_corrente->tempoExec += taskEnd - taskStart;
		task_corrente->ativacoes++;
		taskStart = 0;
		taskEnd = 0;
	}
}

task_t *scheduler(){
	if (ready > 0){
		task_t *task_selecionada = ready;
 		task_t *task_next = ready->next;
		
// Seleciona task
		do{
			if (task_next->prioridade_dinamica < task_selecionada->prioridade_dinamica){
				task_selecionada = task_next;
			}
			task_next = task_next->next;
		}while(task_next != ready);
// Envelhecimento
		task_next = ready;
		do{
			if (task_next->tid != task_selecionada->tid){
				task_next->prioridade_dinamica = task_next->prioridade_dinamica + alpha;
			}else{
				task_selecionada->prioridade_dinamica = task_selecionada->prioridade_estatica;
			}
			task_next = task_next->next;
		}while (task_next != ready);

		return task_selecionada;
	}else return 0;
}

void dispatcher_body (){ // dispatcher é uma tarefa
	#ifdef DEBUG
	printf("dispatcher: inicioucom fila size = %d\n",queue_size((queue_t *) ready));
	#endif
	while ( ((queue_t *) ready) > 0 ){
		next = scheduler(); // scheduler é uma função
		if (next){
			#ifdef DEBUG
			printf("task_dispatcher-%p \n",next);
			#endif
			quantum=20;
			taskStart=systime();
			queue_remove((queue_t **) &ready,(queue_t*) next);
			// ações antes de lançar a tarefa "next", se houverem
			task_switch (next); // transfere controle para a tarefa "next"
			// ações após retornar da tarefa "next", se houverem
		}
	}
	task_exit(0); // encerra a tarefa dispatcher
}




// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void pingpong_init (){
	// Desativa o buffer da saida padrao (stdout), usado pela função printf
	setvbuf (stdout, 0, _IONBF, 0);

	id = 0;
	task_main.next = NULL;
	task_main.prev = NULL;
	task_main.tid = id++;
	task_main.taskUser=TRUE;


	// Referente ao contexto
	ucontext_t context;
	getcontext(&context);

	task_main.context = context;
	task_corrente = &task_main;

	// Dispatcher
	task_create(&dispatcher, (void*) (dispatcher_body),NULL);
	dispatcher.taskUser=FALSE;
	

	action.sa_handler = ticks ;
	sigemptyset (&action.sa_mask) ;
	action.sa_flags = 0 ;
	if (sigaction (SIGALRM, &action, 0) < 0){
		perror ("Erro em sigaction: ") ;
		exit (1) ;
	}
	// ajusta valores do temporizador
	timer.it_value.tv_usec = 1000 ; // primeiro disparo, em micro-segundos
	timer.it_value.tv_sec = 0 ; // primeiro disparo, em segundos
	timer.it_interval.tv_usec = 1000 ; // disparos subsequentes, em micro-segundos
	timer.it_interval.tv_sec = 0 ; // disparos subsequentes, em segundos
	// arma o temporizador ITIMER_REAL (vide man setitimer)
	if (setitimer (ITIMER_REAL, &timer, 0) < 0){
		perror ("Erro em setitimer: ") ;
		exit (1) ;
	}

}

// gerência de tarefas =========================================================

// Cria uma nova tarefa. Retorna um ID> 0 ou erro.
int task_create (task_t *task,			// descritor da nova tarefa
                 void (*start_func)(void *),	// funcao corpo da tarefa
                 void *arg){			// argumentos para a tarefa
	
	task->next = NULL;
	task->prev = NULL;
	task->tid = id++;
	task->flag=FALSE;

	ucontext_t context;
	getcontext(&context);

	char *stack;
	stack = malloc(STACKSIZE);
	if(stack){
		context.uc_stack.ss_sp = stack;
		context.uc_stack.ss_size = STACKSIZE;
		context.uc_stack.ss_flags = 0;
		context.uc_link = 0;
	}else{
		perror("Erro na pilha");
		return (-1);
	}

	// Cria contexto
	makecontext(&context, (void*) (*start_func), 1, arg);
	task->context = context;

	if (task != &dispatcher){
		queue_append((queue_t **) &ready,(queue_t*) task);
		task->prioridade_estatica = 0;
		task->prioridade_dinamica = 0;
		task->taskUser=TRUE;
	}
	task->ativacoes = 0;
	task-> tempoExec = 0;

	#ifdef DEBUG
	printf("task_create: criou tarefa %d\n",task->tid);
	#endif	
}

// Termina a tarefa corrente, indicando um valor de status encerramento
void task_exit (int exitCode){
	#ifdef DEBUG
	printf("task_exit: tarefa %d sendo encerrada \n",task_corrente->tid);
	#endif
	task_corrente->flag=TRUE;

	printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations\n",task_corrente->tid, systime(), task_corrente->tempoExec, task_corrente->ativacoes);

	if (task_corrente == &dispatcher){
		task_switch(&task_main);
	}
	else {
		queue_remove((queue_t **) &ready,(queue_t*)task_corrente);
		task_switch(&dispatcher);
	}
}

// alterna a execução para a tarefa indicada
int task_switch (task_t *task){
	task_t *aux = task_corrente;
	task_corrente = task;

	#ifdef DEBUG
	printf("task_swap: trocando contexto %d -> %d\n",aux->tid,task->tid);
	#endif
	if(!task->flag){
		swapcontext(&aux->context,&task->context);
	return 0;
	}
	else
		return -1;
}

// retorna o identificador da tarefa corrente (main eh 0)
int task_id (){
	return task_corrente->tid;
}

// suspende uma tarefa, retirando-a de sua fila atual, adicionando-a à fila
// queue e mudando seu estado para "suspensa"; usa a tarefa atual se task==NULL
void task_suspend (task_t *task, task_t **queue){
}

// acorda uma tarefa, retirando-a de sua fila atual, adicionando-a à fila de
// tarefas prontas ("ready queue") e mudando seu estado para "pronta"
void task_resume (task_t *task){
}

// operações de escalonamento ==================================================

// libera o processador para a próxima tarefa, retornando à fila de tarefas
// prontas ("ready queue")
void task_yield (){
	//printf("task_yield-%p \n", task_corrente);
	if(task_corrente!= &task_main)
		queue_append((queue_t**)&ready,(queue_t*)task_corrente);
	task_switch(&dispatcher);
}

// define a prioridade estática de uma tarefa (ou a tarefa atual)
void task_setprio (task_t *task, int prio){
	 if (task == NULL){
		task_corrente->prioridade_estatica = prio;
		task_corrente->prioridade_dinamica = prio;
	}else if (task->prioridade_estatica > -20 && task->prioridade_estatica < 20){
		task->prioridade_estatica = prio;
		task->prioridade_dinamica = prio;
	}
}

// retorna a prioridade estática de uma tarefa (ou a tarefa atual)
int task_getprio (task_t *task){
	int num;
	 if (task == NULL) num = task_corrente->prioridade_estatica;
		else num = task->prioridade_estatica;
	return num;


}

// operações de sincronização ==================================================

// a tarefa corrente aguarda o encerramento de outra task
int task_join (task_t *task){
	return 0;
}

// operações de gestão do tempo ================================================

// suspende a tarefa corrente por t segundos
void task_sleep (int t){
}

// retorna o relógio atual (em milisegundos)
unsigned int systime (){
	return time;
}

// operações de IPC ============================================================

// semáforos

// cria um semáforo com valor inicial "value"
int sem_create (semaphore_t *s, int value){
	return 0;
}

// requisita o semáforo
int sem_down (semaphore_t *s){
	return 0;
}

// libera o semáforo
int sem_up (semaphore_t *s){
	return 0;
}

// destroi o semáforo, liberando as tarefas bloqueadas
int sem_destroy (semaphore_t *s){
	return 0;
}

// mutexes

// Inicializa um mutex (sempre inicialmente livre)
int mutex_create (mutex_t *m){
	return 0;
}

// Solicita um mutex
int mutex_lock (mutex_t *m){
	return 0;
}

// Libera um mutex
int mutex_unlock (mutex_t *m){
	return 0;
}

// Destrói um mutex
int mutex_destroy (mutex_t *m){
	return 0;
}

// barreiras

// Inicializa uma barreira
int barrier_create (barrier_t *b, int N){
	return 0;
}

// Chega a uma barreira
int barrier_join (barrier_t *b){
	return 0;
}

// Destrói uma barreira
int barrier_destroy (barrier_t *b){
	return 0;
}

// filas de mensagens

// cria uma fila para até max mensagens de size bytes cada
int mqueue_create (mqueue_t *queue, int max, int size){
	return 0;
}

// envia uma mensagem para a fila
int mqueue_send (mqueue_t *queue, void *msg){
	return 0;
}

// recebe uma mensagem da fila
int mqueue_recv (mqueue_t *queue, void *msg){
	return 0;
}

// destroi a fila, liberando as tarefas bloqueadas
int mqueue_destroy (mqueue_t *queue){
	return 0;
}

// informa o número de mensagens atualmente na fila
int mqueue_msgs (mqueue_t *queue){
	return 0;
}
