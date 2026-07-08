#ifndef SIM_ETAPAS
#define SIM_ETAPAS

#include "atividades.h"

typedef struct etapa {
    int id;
    char nome[100];
    int quant_atv;
    int cap_eta;          
    int ocupacao_atual;   
    float taxa_falha_ini; 
    int total_falhas;
    int produtos_processados;
    int tempo_minimo;
    int tempo_maximo;
    int tempo_total_gasto;
    int tempo_total_fila;
    fila_atividades atividades;
    struct etapa *next;
    struct etapa *prev;
} etapa;

void inserir_etapa(etapa **head, int id, int cap, float falha_ini, char *nome, int q_atv);
void liberar_etapas(etapa *head);

#endif