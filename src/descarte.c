#include "descarte.h"
#include "utils.h"

void push_descarte(produto *p) {
    no_descarte *novo = xmalloc(sizeof(no_descarte));
    novo->prod = p;
    novo->prox = pilha_descarte_global;
    pilha_descarte_global = novo;
}