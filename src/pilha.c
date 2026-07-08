#include "produtos.h"
#include "pilha.h"
#include "atividades.h"
#include "etapas.h"
#include <stdlib.h>
#include <string.h>
#include "utils.h"

void push_historico(produto *p, atividade *ativ, etapa *eta, int t_fila, int t_ini, int t_fim, int status) {
    no_pilha *novo = xmalloc(sizeof(no_pilha));
    novo->atividade_id = ativ->id;
    strncpy(novo->nome_atividade, ativ->nome, sizeof(novo->nome_atividade)-1);
    novo->nome_atividade[sizeof(novo->nome_atividade)-1] = '\0';
    novo->tick_fila = t_fila;
    novo->tick_inicio = t_ini;
    novo->tick_fim = t_fim;
    novo->status_resultado = status;
    novo->prox = p->historico;
    novo->etapa_id = eta->id;
    strncpy(novo->nome_etapa, eta->nome, sizeof(novo->nome_etapa)-1);
    novo->nome_etapa[sizeof(novo->nome_etapa)-1] = '\0';
    novo->tentativa_num = p->tentativa_atual; 
    p->historico = novo;
}

void pop_historico(produto *p) {
    if (p->historico) {
        no_pilha *aux = p->historico;
        p->historico = p->historico->prox;
        free(aux);
    }
}

no_pilha* inverter_historico(no_pilha *topo) {
    no_pilha *anterior = NULL;
    no_pilha *atual = topo;
    no_pilha *proximo = NULL;
    while (atual != NULL) {
        proximo = atual->prox;  
        atual->prox = anterior; 
        anterior = atual;       
        atual = proximo;        
    }
    return anterior; 
}
void liberar_historico(no_pilha *head){
    no_pilha *aux;
    while(head){
        aux = head;
        head = head->prox;
        free(aux);
    }
}