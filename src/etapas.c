#include "etapas.h"
#include <stdlib.h>
#include <string.h>
#include "utils.h"

void inserir_etapa(etapa **head, int id, int cap, float falha_ini, char *nome, int q_atv) {
    etapa *novo = xmalloc(sizeof(etapa));
    strncpy(novo->nome, nome, sizeof(novo->nome)-1);
    novo->nome[sizeof(novo->nome)-1] = '\0';
    novo->id = id;
    novo->quant_atv = q_atv;
    novo->cap_eta = cap;
    novo->ocupacao_atual = 0;
    novo->taxa_falha_ini = falha_ini;
    novo->total_falhas = 0;
    novo->produtos_processados = 0;
    novo->tempo_minimo = 999999; 
    novo->tempo_maximo = 0;
    novo->tempo_total_gasto = 0;
    novo->tempo_total_fila = 0;
    novo->atividades.start = NULL;
    novo->atividades.end = NULL;
    novo->next = NULL;
    novo->prev = NULL;
    if (!(*head)) *head = novo;
    else {
        etapa *aux = *head;
        while (aux->next) aux = aux->next;
        aux->next = novo;
        novo->prev = aux;
    }
}

void liberar_etapas(etapa *head) {
    etapa *aux;
    while(head){
        aux = head;
        head = head->next;
        liberar_atividades(&aux->atividades);
        free(aux);
    }
}