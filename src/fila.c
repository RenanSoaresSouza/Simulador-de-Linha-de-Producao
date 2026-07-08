#include <stdlib.h>
#include "fila.h"
#include "utils.h"

void enqueue_entrada(fila_entrada *fila, produto *p) {
    no_fila *novo = xmalloc(sizeof(no_fila));
    novo->prod = p;
    novo->next = NULL;
    if (!fila->start) {
        fila->start = novo;
        fila->end = novo;
    } else {
        fila->end->next = novo;
        fila->end = novo;
    }
}

produto *dequeue_entrada(fila_entrada *fila) {
    if (!fila->start) return NULL;
    no_fila *aux = fila->start;
    produto *p = aux->prod;
    fila->start = fila->start->next;
    if (!fila->start) fila->end = NULL;
    free(aux);
    return p;
}