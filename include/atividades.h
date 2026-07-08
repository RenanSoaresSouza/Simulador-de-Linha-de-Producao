#ifndef SIM_ATIVIDADES_H
#define SIM_ATIVIDADES_H

typedef struct atividade {
    char nome[50];
    float falha;
    int duracao;
    int id;
    int itens_processados;
    int tempo_total_fila;
    int tempo_total_execucao;
    struct atividade *next;
} atividade;

typedef struct fila_atividades {
    atividade *start;
    atividade *end;
} fila_atividades;

void inserir_atividade(fila_atividades *fila, const char *nome, float falha, int duracao, int id);
void liberar_atividades(fila_atividades *fila);

#endif