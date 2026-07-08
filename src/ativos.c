#include "ativos.h"
#include "produtos.h"
#include <stdlib.h>
#include "utils.h"

void remover_ativos(ativos **head, int id) {
    ativos *aux = *head, *ant = NULL;
    while (aux && aux->prod->id != id) {
        ant = aux;
        aux = aux->next;
    }
    if (aux) {
        if (!ant) *head = aux->next;
        else ant->next = aux->next;
        free(aux);
    }
}

void inserir_ativos(ativos **head, produto *p) {
    ativos *novo = xmalloc(sizeof(ativos));
    novo->prod = p;
    novo->next = *head;
    (*head) = novo;
}