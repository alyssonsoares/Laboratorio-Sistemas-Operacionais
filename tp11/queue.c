#include <stdio.h>
#include <stdlib.h>
#include "queue.h"


void queue_append (queue_t **queue, queue_t *elem){
	
	queue_t *aux;
	if ( elem == NULL); //Elemento Não existe
	else if ( elem->prev !=NULL || elem->next != NULL); //Elemento já faz parte de outra fila
	else if (*queue == NULL) { // Fila Vazia
		aux = elem;
		aux->prev = elem;
		aux->next = elem;
		*queue = aux;
	}
	else{ // Fila Nao fazia
		aux = (*queue)->prev;
		elem->prev = aux;
		elem->next = (*queue);
		aux->next = elem;
		(*queue)->prev = elem;
	}
}

int queue_size( queue_t *queue){
	if(queue==NULL){
		return 0;
	}
	int cont = 1;
	queue_t *queue_aux = queue, *first=queue;
	while(queue_aux->next!=first){
		queue_aux=queue_aux->next;
		cont++;
	}
	return cont;
}

void queue_print (char *name, queue_t *queue, void print_elem (void *) ) {
	queue_t *queue_aux = queue, *first = queue;
		if (queue != NULL) {
			printf("%s [", name);
			print_elem((void *)queue_aux);
			while (queue_aux->next != first) {
				queue_aux = queue_aux->next;
				print_elem((void *)queue_aux);
			}
			printf("]\n");
		}
	else
		printf("%s []\n", name);
}

queue_t *queue_remove (queue_t **queue, queue_t *elem) {
	queue_t *first, *queue_aux, *aux;
		if (elem == NULL) {
			return NULL;
		}
		if (*queue == NULL) {
			return NULL;
		}
		else {
			first = *queue;
			queue_aux = *queue;
			if (elem == first) {
				if (queue_size(*queue) == 1)
					*queue=NULL;
				else {
					queue_t *next = first->next;
					queue_t *prev = first->prev;
					next->prev = prev;
					prev->next = next;
					first = next;
					*queue = first;
				}
				elem->prev = NULL;
				elem->next = NULL;
				return elem;
			}
		else {
			while (queue_aux->next != first) {
				aux = queue_aux->next;
				if (elem == aux) {
					queue_t *next = aux->next;
					queue_t *prev = aux->prev;
					next->prev = prev;
					prev->next = next;
					elem->prev = NULL;
					elem->next = NULL;
					return elem;
				}
				queue_aux = queue_aux->next;
			}
		}
	}
	return NULL;
}