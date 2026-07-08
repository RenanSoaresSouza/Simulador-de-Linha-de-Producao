#ifndef SIM_PILHA
#define SIM_PILHA

typedef struct produto produto;
typedef struct atividade atividade;
typedef struct etapa etapa;

typedef struct no_pilha {
    int atividade_id;
    int etapa_id;
    char nome_etapa[50];
    char nome_atividade[50];
    int tentativa_num; 
    int tick_fila;
    int tick_inicio;
    int tick_fim;
    int status_resultado; 
    struct no_pilha *prox;
} no_pilha;

void push_historico(produto *p, atividade *ativ, etapa *eta, int t_fila, int t_ini, int t_fim, int status);
void pop_historico(produto *p);
no_pilha* inverter_historico(no_pilha *topo);
void liberar_historico(no_pilha *head);

#endif