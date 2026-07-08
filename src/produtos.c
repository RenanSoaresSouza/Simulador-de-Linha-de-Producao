#include "ativos.h"
#include "produtos.h"
#include <stdlib.h>
#include "pilha.h"

void liberar_produtos(ativos *lista){
    ativos *aux;
    while (lista){
        aux = lista;
        lista = lista->next;
        liberar_historico(aux->prod->historico);
        free(aux->prod);
        free(aux);
    }
}