#include "descarte.h"
#include "utils.h"
#include "produtos.h"
#include "pilha.h"

void push_descarte(produto *p) {
    no_descarte *novo = xmalloc(sizeof(no_descarte));
    novo->prod = p;
    novo->prox = pilha_descarte_global;
    pilha_descarte_global = novo;
}

void liberar_descarte(no_descarte *pilha){
    no_descarte *aux;
    while (pilha){
        aux = pilha;
        pilha = pilha->prox;
        liberar_historico(aux->prod->historico);
        free(aux->prod);
        free(aux);
    }
}