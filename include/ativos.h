#ifndef SIM_ATIVOS
#define SIM_ATIVOS

typedef struct produto produto;

typedef struct ativos {
    produto *prod;
    struct ativos *next;
} ativos;

void inserir_ativos(ativos **head, produto *p);
void remover_ativos(ativos **head, int id);

#endif