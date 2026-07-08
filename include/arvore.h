#ifndef SIM_ARVORE
#define SIM_ARVORE

typedef struct produto produto;

typedef struct produtos_log_tree {
    produto *prod;
    struct produtos_log_tree *left;
    struct produtos_log_tree *right;
} produtos_log_tree;

produtos_log_tree *inserir_arvore(produtos_log_tree *raiz, produto *p);
produtos_log_tree *remover_arvore(produtos_log_tree *raiz, int id);
void liberar_arvore(produtos_log_tree *raiz);

#endif