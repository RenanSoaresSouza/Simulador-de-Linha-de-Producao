#ifndef SIM_FILA
#define SIM_FILA

typedef struct produto produto;

typedef struct no_fila {
    produto *prod;
    struct no_fila *next;
} no_fila;

typedef struct fila_entrada {
    no_fila *start;
    no_fila *end;
} fila_entrada;

void enqueue_entrada(fila_entrada *fila, produto *p);
produto *dequeue_entrada(fila_entrada *fila);
void liberar_fila_entrada(fila_entrada *fila);

#endif