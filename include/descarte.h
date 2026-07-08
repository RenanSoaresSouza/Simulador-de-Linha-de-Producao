#ifndef SIM_DESCARTE
#define SIM_DESCARTE

#include <stdlib.h>

typedef struct produto produto;

typedef struct no_descarte {
    produto *prod;
    struct no_descarte *prox;
} no_descarte;

extern no_descarte *pilha_descarte_global;
extern int total_falhas_sistema;
extern int total_concluidos_sistema;

void push_descarte(produto *p);
void liberar_descarte(no_descarte *pilha);

#endif
