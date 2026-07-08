#include "arvore.h"
#include "produtos.h"
#include <stdlib.h>
#include "utils.h"

produtos_log_tree *inserir_arvore(produtos_log_tree *raiz, produto *p) {
    if (!(raiz)) {
        produtos_log_tree *novo = xmalloc(sizeof(produtos_log_tree));
        novo->prod = p;
        novo->left = NULL;
        novo->right = NULL;
        return novo;
    }
    if (p->id < raiz->prod->id) raiz->left = inserir_arvore(raiz->left, p);
    else raiz->right = inserir_arvore(raiz->right, p);
    return raiz;
}

produtos_log_tree *remover_arvore(produtos_log_tree *raiz, int id) {
    if (!raiz) return NULL;
    if (id < raiz->prod->id) raiz->left = remover_arvore(raiz->left, id);
    else if (id > raiz->prod->id) raiz->right = remover_arvore(raiz->right, id);
    else {
        if (!raiz->left) {
            produtos_log_tree *temp = raiz->right;
            free(raiz);
            return temp;
        } else if (!raiz->right) {
            produtos_log_tree *temp = raiz->left;
            free(raiz);
            return temp;
        }
        produtos_log_tree *temp = raiz->right;
        while (temp && temp->left) temp = temp->left;
        raiz->prod = temp->prod;
        raiz->right = remover_arvore(raiz->right, temp->prod->id);
    }
    return raiz;
}
void liberar_arvore(produtos_log_tree *raiz) {
    if (raiz == NULL)
        return;

    liberar_arvore(raiz->left);
    liberar_arvore(raiz->right);

    free(raiz);
}