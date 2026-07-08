#ifndef SIM_UTILS
#define UTILS

#include <stdlib.h>
typedef struct etapa etapa;
typedef struct fila_entrada fila_entrada;
typedef struct ativos ativos;
// ==========================================
//              FUNÇÕES NECESSÁRIAS
// ==========================================

void *xmalloc(size_t size);
void imprimir_painel(int tick_atual, etapa *fila_etapas, fila_entrada *f_entrada, ativos *fila_ativos);
void carregar_arquivo(char *nome_file, etapa **fila_etapas, int *qtd_total, int *vazao, char *nome, int *semente, int *duracao);

#endif