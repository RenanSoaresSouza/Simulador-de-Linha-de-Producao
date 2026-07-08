#include "atividades.h" 
#include <stdlib.h>
#include <string.h>
#include "utils.h"

void inserir_atividade(fila_atividades *fila, const char *nome, float falha, int duracao, int id) {
    atividade *novo = xmalloc(sizeof(atividade));
    strncpy(novo->nome, nome, sizeof(novo->nome)-1);
    novo->nome[sizeof(novo->nome)-1] = '\0';
    novo->falha = falha;
    novo->duracao = duracao;
    novo->id = id;
    novo->itens_processados = 0;
    novo->tempo_total_fila = 0;
    novo->tempo_total_execucao = 0;
    novo->next = NULL;
    if (!(fila->start)) {
        fila->start = novo;
        fila->end = novo;
    } else {
        fila->end->next = novo;
        fila->end = novo;
    }
}

void liberar_atividades(fila_atividades *fila) {
    atividade *aux;
    while (fila->start){
        aux = fila->start;
        fila->start = fila->start->next;
        free(aux);
    }
    fila->end = NULL;
}